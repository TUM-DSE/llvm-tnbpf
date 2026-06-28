//
// Created by deniz on 6/15/26.
//

#include "BPF.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "SCEVCanonicalPrintVisitor.h"
#include <atomic>
#define DEBUG_TYPE "bpf-loop-tagging"

using namespace llvm;

namespace {
std::atomic<unsigned long long> loop_id = 0;
std::atomic<unsigned long long> lookup_table_id = 0;
std::atomic<unsigned long long> sym_ref_instruction_id = 0;
//TODO: Move these conditions somewhere more accessible

enum PCSLoopInfoType {
    loop_cond_branch,
    loop_cond_before,
    loop_cond_instr,
    loop_body_begin,
};




class SymbolicBoundTranslationTableEntry {
private:
  virtual llvm::Constant *emitToTable(llvm::LLVMContext &context);
  virtual uint64_t getEntryType();
public:
    enum EntryTypes {
      Instruction,
      Constant,
      FunctionParameter
    };

  llvm::Constant *getTableEntry(llvm::LLVMContext &context) {

    return llvm::ConstantStruct::getAnon({
      llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, getEntryType())),
      emitToTable(context)
    });
  }
  virtual ~SymbolicBoundTranslationTableEntry() {};


};

class SBTFunctionArgEntry : public SymbolicBoundTranslationTableEntry {
  llvm::Argument *arg;
private:
  llvm::Constant *emitToTable(llvm::LLVMContext &context) override {
    return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, arg->getArgNo()));
  }
  uint64_t getEntryType() override {
  return EntryTypes::FunctionParameter;
  }
  public:
  SBTFunctionArgEntry(llvm::Argument *arg) : arg(arg) {}

  ~SBTFunctionArgEntry() override {}
};
class SBTInstructionEntry : public SymbolicBoundTranslationTableEntry {

llvm::Instruction *inst;
private:
  llvm::Constant *emitToTable(llvm::LLVMContext &context) override {
    llvm::MDBuilder mb(context);
    uint64_t inst_tag_id = sym_ref_instruction_id.fetch_add(1);
    auto loop_name = std::string("_sym_instr_db");
    const auto module_name = inst->getModule()->getModuleIdentifier();
    loop_name.insert(0, module_name);
    auto old_mdnode = inst->getMetadata("pcsections");
    llvm::MDNode *node = mb.createPCSections({
          {loop_name, {
            llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, inst_tag_id)),
          }}
    });
    if (old_mdnode) {
      node = llvm::MDNode::concatenate(old_mdnode, node);
    }
    inst->setMetadata("pcsections", node);
    return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, inst_tag_id));
  }
  uint64_t getEntryType() override {
  return EntryTypes::Instruction;
  }
public:

  SBTInstructionEntry(llvm::Instruction *inst) : inst(inst) {

  }

  ~SBTInstructionEntry() override {};
};

class SBTConstantEntry : public SymbolicBoundTranslationTableEntry {

llvm::Constant *c;
private:
  llvm::Constant *emitToTable(llvm::LLVMContext &context) override {
    //TODO: if c is a global constant value, this just copies the whole entire global constant to c
    return c;
  }
  uint64_t getEntryType() override {
    return EntryTypes::Constant;
  }
public:
  SBTConstantEntry(llvm::Constant *c) : c(c) {

  }

  ~SBTConstantEntry() override {};
};
class SymbolicBoundTranslationTable {
    private:
    ScalarEvolution &SE;
    const llvm::SCEV *symbolic_bound;
  // TODO: I don't actually know if we need the * in here or if C++ sorts out ownership for us
    std::vector<SymbolicBoundTranslationTableEntry *> dependencies;
    std::string resultString;
    public:
    std::vector<Value *> original_values;
    SymbolicBoundTranslationTable(ScalarEvolution &SE, const SCEV *scev_from) : SE(SE) {
        this->symbolic_bound = scev_from;
        SCEVCanonicalPrintVisitor visitor;
        visitor.visit(scev_from);
        auto results = visitor.collectResults();
        auto translation_map = results.first;
        //debugging purposes
        this->original_values = translation_map;
        resultString = results.second;
        //now, translate results to our symbolic table format

        for (unsigned i = 0; i < translation_map.size();i++) {
          auto val = translation_map[i];
          if (auto func_arg = dyn_cast<Argument>(val)) {
            dependencies.push_back(new SBTFunctionArgEntry(func_arg));
          } else if (auto constant = dyn_cast<Constant>(val)) {
            dependencies.push_back(new SBTConstantEntry(constant));
          } else if (auto instr = dyn_cast<Instruction>(val)) {
            dependencies.push_back(new SBTInstructionEntry(instr));
          } else {
            llvm_unreachable("Unhandled value type encountered in SCEV!");
          }
        }
    }

    void tagAndEmitTable(llvm::Function *function_to_tag, llvm::LLVMContext &context, llvm::MDBuilder &mb, uint64_t loop_number) {
      auto loop_name = std::string("_loopdb_symmax");
      const auto module_name = function_to_tag->begin()->getModule()->getModuleIdentifier();
      loop_name.insert(0, module_name);
      auto old_mdnode = function_to_tag->getMetadata("pcsections");

      llvm::SmallVector<llvm::Constant *> entries;

      entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, loop_number)));
      //first, embed the string itself
      entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, this->getResultString().size())));
      entries.push_back(ConstantDataArray::getString(context, getResultString()));
      //now, we need to encode our table
      entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, this->dependencies.size())));

      for (auto dep : dependencies) {
        auto to_emit = dep->getTableEntry(context);
        // https://stackoverflow.com/a/14608251
        // More specifically, https://stackoverflow.com/questions/14608250/how-can-i-find-the-size-of-a-type#comment125528828_14608251
        uint64_t const_size = function_to_tag->begin()->getModule()->getDataLayout().getTypeAllocSize(to_emit->getType());
        entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, const_size)));
        entries.push_back(to_emit);
      }

      llvm::MDNode *node = mb.createPCSections({
            {loop_name, entries}
      });
      if (old_mdnode) {
        node = llvm::MDNode::concatenate(old_mdnode, node);
      }
      function_to_tag->setMetadata("pcsections", node);
    }

    ~SymbolicBoundTranslationTable() {
      LLVM_DEBUG(dbgs() << "translation table destructor called\n");
        for (auto *dep : dependencies) {
          delete dep;
        }
    }

    std::string getResultString() {
      return resultString;
    }
};
class LoopTemplate {
public:
  uint64_t loop_number;
  std::optional<int64_t> start = {};
  std::optional<int64_t> finish = {};
  std::optional<int64_t> stride = {};
  enum ComparisonType {
    //Bitmasking enabled comparison enum
    //        < = >
    FALSE,  //0 0 0
    GT,     //0 0 1
    EQ,     //0 1 0
    GEQ,    //0 1 1
    LT,     //1 0 0
    NEQ,    //1 0 1
    LEQ,    //1 1 0
    TRUE,   //1 1 1
    C_UNK,
  };
  ComparisonType comp_type = C_UNK;
  std::optional<bool> loopIsAscending= {};
  std::optional<bool> loopTerminates= {};
  //TODO: populate these two as well
  std::optional<uint64_t> exact_exit_count= {};
  std::optional<uint64_t> constant_max_exit_count= {};

  std::optional<SymbolicBoundTranslationTable *> translation_table = {};

  LoopTemplate() {
    this->loop_number = loop_id.fetch_add(1);
  }
  ~LoopTemplate() {
    LLVM_DEBUG(dbgs() << "loop template destructor called\n");
    if (translation_table.has_value()) {
      delete translation_table.value();
    }
  }
};
static void pcSectionTagInstr(llvm::LLVMContext &context, llvm::MDBuilder &mb, PCSLoopInfoType info_type, llvm::Instruction *instr, uint64_t loop_number) {
  auto loop_name = std::string("_loopdb");
  const auto module_name = instr->getModule()->getModuleIdentifier();
  loop_name.insert(0, module_name);
  auto old_mdnode = instr->getMetadata("pcsections");
  llvm::MDNode *node = mb.createPCSections({
        {loop_name, {
          llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, loop_number)),
          llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, info_type)),
        }}
  });
  if (old_mdnode) {
    node = llvm::MDNode::concatenate(old_mdnode, node);
  }
  instr->setMetadata("pcsections", node);
}

static void pcSectionLoopClassifyTag(llvm::LLVMContext &context, llvm::MDBuilder mb, LoopTemplate &templ, llvm::Function *func) {
  auto loop_name = std::string("_loopdb_class");
  //TODO: this is an extremely hacky way to get the module from the function and breaks if the function is currently empty
  const auto module_name = func->begin()->getModule()->getModuleIdentifier();
  loop_name.insert(0, module_name);
  if (templ.translation_table.has_value()) {
    templ.translation_table.value()->tagAndEmitTable(func, context, mb, templ.loop_number);
  }
  auto old_mdnode = func->getMetadata("pcsections");
  auto i64t = llvm::Type::getInt64Ty(context);
  auto i16t = llvm::Type::getInt16Ty(context);
  auto i1t = llvm::Type::getInt1Ty(context);
  // TODO: This is an absolutely horrible way to specify the type
  // Does LLVM have some sort of type annotation that auto-creates LLVM struct
  // types from regular host struct decls?
  llvm::MDNode *node = mb.createPCSections({
        {loop_name, {llvm::ConstantStruct::getAnon({
          llvm::Constant::getIntegerValue(i64t, llvm::APInt(64, templ.loop_number)),
          llvm::Constant::getIntegerValue(i64t, llvm::APInt(64, templ.start.value_or(0))),
          llvm::Constant::getIntegerValue(i64t, llvm::APInt(64, templ.finish.value_or(0))),
          llvm::Constant::getIntegerValue(i64t, llvm::APInt(64, templ.stride.value_or(0))),
          llvm::Constant::getIntegerValue(i64t, llvm::APInt(64, templ.exact_exit_count.value_or(0))),
          llvm::Constant::getIntegerValue(i64t, llvm::APInt(64, templ.constant_max_exit_count.value_or(0))),
          llvm::Constant::getIntegerValue(i16t, llvm::APInt(16, templ.comp_type)),
          //llvm::Constant::getIntegerValue(i16t, llvm::APInt(16, templ.loop_type)),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopIsAscending.value_or(false))),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopTerminates.value_or(false))),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopIsAscending.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.start.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.finish.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.stride.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopTerminates.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.exact_exit_count.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.constant_max_exit_count.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.translation_table.has_value()))
        })}}
  });
  if (old_mdnode) {
    node = llvm::MDNode::concatenate(old_mdnode, node);
  }
  func->setMetadata("pcsections", node);
}

  struct BPFLoopTagging : public LoopPass {
    static char ID;
    BPFLoopTagging() : LoopPass(ID) {}
  public:
    bool runOnLoop(Loop *L, LPPassManager &LPM) override {
      LLVM_DEBUG(dbgs() << "begin bpf loop pass on loop:" << "\n");
      auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      auto &loop_info = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
      LLVM_DEBUG(dbgs() << "got analysis" << "\n");

      LoopTemplate loop_meta;


      LLVM_DEBUG(dbgs() << "print entire loop:" << "\n");
      printLoop(*L, dbgs());
      auto lb = L->getBounds(SE);
      if (lb.has_value()) {
        auto unpacked_lb = lb.value();
        switch (unpacked_lb.getDirection()) {
        case Loop::LoopBounds::Direction::Increasing:
          LLVM_DEBUG(dbgs() << "loop increasing" << "\n");
          loop_meta.loopIsAscending = true;
          break;
        case Loop::LoopBounds::Direction::Unknown:
          LLVM_DEBUG(dbgs() << "loop unknown" << "\n");
          break;
        case Loop::LoopBounds::Direction::Decreasing:
          LLVM_DEBUG(dbgs() << "loop decreasing" << "\n");
          loop_meta.loopIsAscending = false;
          break;
        }

        auto &initial = unpacked_lb.getInitialIVValue();
        auto initial_scev = SE.getSCEV(&initial);
        //TODO: Maybe also get info about the initial, end and stride values as string SCEVs and pack them in
        //with the same architecture?
        if (ConstantInt *init_int = dyn_cast<ConstantInt>(&initial)) {
          LLVM_DEBUG(dbgs() << "print initial as constant int:" << "\n");
          LLVM_DEBUG(dbgs() << init_int->getValue());
          //TODO: what if we overflow / underflow?
          loop_meta.start = init_int->getSExtValue();
        }
        auto &end = unpacked_lb.getFinalIVValue();
        auto step = unpacked_lb.getStepValue();
        if (ConstantInt *final_int = dyn_cast<ConstantInt>(&end)) {
          //TODO: what if we overflow / underflow?
          loop_meta.finish = final_int->getSExtValue();
        }
        if (ConstantInt *step_int = dyn_cast<ConstantInt>(step)) {
          //TODO: what if we overflow / underflow?
          loop_meta.stride = step_int->getSExtValue();
        }
      //TODO: maybe just take the CmpInst predicate into our struct instead of reinventing the wheel?
        switch (unpacked_lb.getCanonicalPredicate()) {
        case CmpInst::FCMP_FALSE:
          loop_meta.comp_type = LoopTemplate::FALSE;
          loop_meta.loopTerminates = true;
          break;
        case CmpInst::FCMP_OEQ:
          loop_meta.comp_type = LoopTemplate::EQ;
          break;
        case CmpInst::FCMP_OGT:
          loop_meta.comp_type = LoopTemplate::GT;
          break;
        case CmpInst::FCMP_OGE:
          loop_meta.comp_type = LoopTemplate::GEQ;
          break;
        case CmpInst::FCMP_OLT:
          loop_meta.comp_type = LoopTemplate::LT;
          break;
        case CmpInst::FCMP_OLE:
          loop_meta.comp_type = LoopTemplate::LEQ;
          break;
        case CmpInst::FCMP_ONE:
          loop_meta.comp_type = LoopTemplate::NEQ;
          break;
        case CmpInst::FCMP_ORD:
          break;
        case CmpInst::FCMP_UNO:
          break;
        case CmpInst::FCMP_UEQ:
          loop_meta.comp_type = LoopTemplate::EQ;
          break;
        case CmpInst::FCMP_UGT:
          loop_meta.comp_type = LoopTemplate::GT;
          break;
        case CmpInst::FCMP_UGE:
          loop_meta.comp_type = LoopTemplate::GEQ;
          break;
        case CmpInst::FCMP_ULT:
          loop_meta.comp_type = LoopTemplate::LT;
          break;
        case CmpInst::FCMP_ULE:
          loop_meta.comp_type = LoopTemplate::LEQ;
          break;
        case CmpInst::FCMP_UNE:
          loop_meta.comp_type = LoopTemplate::NEQ;
          break;
        case CmpInst::FCMP_TRUE:
          loop_meta.comp_type = LoopTemplate::TRUE;
          loop_meta.loopTerminates = false;
          break;
        case CmpInst::BAD_FCMP_PREDICATE:
          break;
        case CmpInst::ICMP_EQ:
          loop_meta.comp_type = LoopTemplate::EQ;
          break;
        case CmpInst::ICMP_NE:
          loop_meta.comp_type = LoopTemplate::NEQ;
          break;
        case CmpInst::ICMP_UGT:
          loop_meta.comp_type = LoopTemplate::GT;
          break;
        case CmpInst::ICMP_UGE:
          loop_meta.comp_type = LoopTemplate::GEQ;
          break;
        case CmpInst::ICMP_ULT:
          loop_meta.comp_type = LoopTemplate::LT;
          break;
        case CmpInst::ICMP_ULE:
          loop_meta.comp_type = LoopTemplate::LEQ;
          break;
        case CmpInst::ICMP_SGT:
          loop_meta.comp_type = LoopTemplate::GT;
          break;
        case CmpInst::ICMP_SGE:
          loop_meta.comp_type = LoopTemplate::GEQ;
          break;
        case CmpInst::ICMP_SLT:
          loop_meta.comp_type = LoopTemplate::LT;
          break;
        case CmpInst::ICMP_SLE:
          loop_meta.comp_type = LoopTemplate::LEQ;

          break;
        case CmpInst::BAD_ICMP_PREDICATE:
          break;
        }



      } else {
        LLVM_DEBUG(dbgs() << "failed to get loop bounds" << "\n");
      }

      //I haven't got the slightest clue why these functions in particular give results but I am not complaining
      auto exact_end = SE.getBackedgeTakenCount(L, ScalarEvolution::Exact);
      LLVM_DEBUG(dbgs() << "exact exit count: ");
      LLVM_DEBUG(exact_end->print(dbgs()));
      LLVM_DEBUG(dbgs() << "\n");
      auto const_max_end = SE.getBackedgeTakenCount(L, ScalarEvolution::ConstantMaximum);
      LLVM_DEBUG(dbgs() << "constant max exit count: ");
      LLVM_DEBUG(const_max_end->print(dbgs()));
      LLVM_DEBUG(dbgs() << "\n");
      auto sym_max_end = SE.getSymbolicMaxBackedgeTakenCount(L);
      LLVM_DEBUG(dbgs() << "symbolic max exit count: ");
      LLVM_DEBUG(sym_max_end->print(dbgs()));
      for (auto operand : sym_max_end->operands()) {
        LLVM_DEBUG(dbgs() << "operand: ");
        LLVM_DEBUG(operand.print(dbgs()));
        LLVM_DEBUG(dbgs() << "\n");
      }
      SymbolicBoundTranslationTable *sym_max_end_standardized = new SymbolicBoundTranslationTable(SE, sym_max_end);
      LLVM_DEBUG(dbgs() << "symbolic max exit count SCEV result string: " << sym_max_end_standardized->getResultString() << "\n");
      LLVM_DEBUG(dbgs() << "results vector:\n");
      loop_meta.translation_table = sym_max_end_standardized;
      for (int i=0;i<loop_meta.translation_table.value()->original_values.size();i++) {
        LLVM_DEBUG(dbgs() << "%" << i << " -> ");
        LLVM_DEBUG(loop_meta.translation_table.value()->original_values[i]->printAsOperand(dbgs()));
        LLVM_DEBUG(dbgs() << "\n");
      }
      LLVM_DEBUG(dbgs() << "end bpf loop pass on loop:" << "\n");
      //TODO: this is a very hacky way to get an LLVM Context
      pcSectionLoopClassifyTag(L->getHeader()->getContext(), llvm::MDBuilder(L->getHeader()->getContext()), loop_meta, L->getHeader()->getParent());

      return false;
    }
    void getAnalysisUsage(AnalysisUsage &Info) const override {
        //TODO: There are two analysis passes called "Machine Loop Info" and "Loop Info"
        // I don't know which level to be working on
        //Info.addRequiredTransitive<LoopInfoWrapperPass>();
      Info.addRequiredTransitive<ScalarEvolutionWrapperPass>();
      getLoopAnalysisUsage(Info);
    }
  };
}

INITIALIZE_PASS_BEGIN(BPFLoopTagging, DEBUG_TYPE, "Tags loops with pc section annotations for later proof generation", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopPass)
INITIALIZE_PASS_END(BPFLoopTagging, DEBUG_TYPE, "Tags loops with pc section annotations for later proof generation", false, false)
char BPFLoopTagging::ID = 0;
LoopPass *llvm::createBPFLoopTaggingPass() {
  return new BPFLoopTagging();
}
