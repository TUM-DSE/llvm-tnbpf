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

#include <atomic>
#define DEBUG_TYPE "bpf-loop-tagging"

using namespace llvm;

namespace {
std::atomic<unsigned long long> loop_id = 0;

//TODO: Move these conditions somewhere more accessible

enum PCSLoopInfoType {
    loop_cond_branch,
    loop_cond_before,
    loop_cond_instr,
    loop_body_begin,
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
    std::optional<uint64_t> exact_exit_count= {};
    std::optional<uint64_t> constant_max_exit_count= {};

    LoopTemplate() {
      this->loop_number = loop_id.fetch_add(1);
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

static void pcSectionLoopClassifyTag(llvm::LLVMContext &context, llvm::MDBuilder mb, LoopTemplate templ, llvm::Function *func) {
  auto loop_name = std::string("_loopdb_class");
  //TODO: this is an extremely hacky way to get the module from the function and breaks if the function is currently empty
  const auto module_name = func->begin()->getModule()->getModuleIdentifier();
  loop_name.insert(0, module_name);

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
          llvm::Constant::getIntegerValue(i16t, llvm::APInt(16, templ.comp_type)),
          //llvm::Constant::getIntegerValue(i16t, llvm::APInt(16, templ.loop_type)),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopIsAscending.value_or(false))),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopTerminates.value_or(false))),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopIsAscending.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.start.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.finish.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.stride.has_value())),
          llvm::Constant::getIntegerValue(i1t, llvm::APInt(1, templ.loopTerminates.has_value())),
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
      loop_meta.loop_number = loop_id.fetch_add(1);


      LLVM_DEBUG(dbgs() << "print entire loop:" << "\n");
      printLoop(*L, dbgs());
      auto lb = L->getBounds(SE);
      if (lb.has_value()) {
        auto unpacked_lb = lb.value();
        switch (unpacked_lb.getDirection()) {
        case Loop::LoopBounds::Direction::Increasing:
          LLVM_DEBUG(dbgs() << "loop increasing" << "\n");
          break;
        case Loop::LoopBounds::Direction::Unknown:
          LLVM_DEBUG(dbgs() << "loop unknown" << "\n");
          break;
        case Loop::LoopBounds::Direction::Decreasing:
          LLVM_DEBUG(dbgs() << "loop decreasing" << "\n");
          break;
        }

        auto &initial = unpacked_lb.getInitialIVValue();
        auto initial_scev = SE.getSCEV(&initial);
        if (ConstantInt *init_int = dyn_cast<ConstantInt>(&initial)) {
          LLVM_DEBUG(dbgs() << "print initial as constant int:" << "\n");
          LLVM_DEBUG(dbgs() << init_int->getValue());
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
      LLVM_DEBUG(dbgs() << "\n");
      LLVM_DEBUG(dbgs() << "end bpf loop pass on loop:" << "\n");
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
