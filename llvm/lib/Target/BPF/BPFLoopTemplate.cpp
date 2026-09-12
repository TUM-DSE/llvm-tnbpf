//
// Created by deniz on 9/8/26.
//

#include "BPFLoopTemplate.h"
#include "SBT/SymbolicBoundTranslationTable.h"
#include "atomic"

#include "llvm/CodeGen/PCSectionHelpers.h"

std::atomic<unsigned long long> loop_id = 0;

BPFLoopTemplate::BPFLoopTemplate() {
  this->comp_type = C_UNK;

  this->loop_number = loop_id.fetch_add(1);
}
BPFLoopTemplate::~BPFLoopTemplate() {

}

void BPFLoopTemplate::pcSectionLoopClassifyTag(Function *func) {
    LLVMContext &context = func->getContext();
    MDBuilder mb(context);

    auto loop_name = std::string("_loopdb_class");
    //TODO: this is an extremely hacky way to get the module from the function and breaks if the function is currently empty
    const auto module_name = func->begin()->getModule()->getModuleIdentifier();
    loop_name.insert(0, module_name);
    if (this->translation_table_measure.has_value()) {
      this->translation_table_measure.value()->tagAndEmitTable(func, this->loop_number);
    }
    auto old_mdnode = initOrGetPCSectionArrayFunction(context, func, loop_name);

    LLVM_DEBUG(dbgs() << "Print mdnode data before pc section loop: \n");
    LLVM_DEBUG(old_mdnode->printTree(dbgs()));
    LLVM_DEBUG(dbgs() << "\n");

    //auto i64t = llvm::Type::getInt64Ty(context);
    //auto i16t = llvm::Type::getInt16Ty(context);
    //auto i1t = llvm::Type::getInt1Ty(context);
    // TODO: This is an absolutely horrible way to specify the type
    // Does LLVM have some sort of type annotation that auto-creates LLVM struct
    // types from regular host struct decls?
    SmallVector<Constant *> entries = {
      embedU64(context, this->loop_number),
      embedU64(context, this->start.value_or(0)),
      embedU64(context, this->finish.value_or(0)),
      embedU64(context, this->stride.value_or(0)),
      embedU64(context, this->exact_exit_count.value_or(0)),
      embedU64(context, this->constant_max_exit_count.value_or(0)),
      this->br_id_tag.value_or(embedU64(context, 0)),
      this->cmp_id_tag.value_or(embedU64(context, 0)),
      embedU16(context, this->comp_type),
      embedU1(context, this->loopIsAscending.value_or(false)),
      embedU1(context, this->loopTerminates.value_or(false)),
      embedU1(context, this->loopIsAscending.has_value()),
      embedU1(context, this->start.has_value()),
      embedU1(context, this->finish.has_value()),
      embedU1(context, this->stride.has_value()),
      embedU1(context, this->loopTerminates.has_value()),
      embedU1(context, this->exact_exit_count.has_value()),
      embedU1(context, this->constant_max_exit_count.has_value()),
      embedU1(context, this->translation_table_measure.has_value()),
      embedU1(context, this->br_id_tag.has_value()),
      embedU1(context, this->cmp_id_tag.has_value()),
      embedU64(context, this->latches.size())
  };
  entries.append(this->latches);
  func->setMetadata("pcsections", appendToPCSectionArray(context, loop_name, old_mdnode, entries));
  LLVM_DEBUG(dbgs() << "Print mdnode data after pcsection loop: \n");
  LLVM_DEBUG(func->getMetadata("pcsections")->printTree(dbgs()));
  LLVM_DEBUG(dbgs() << "\n");
}

void BPFLoopTemplate::importLoopComparison(ICmpInst::Predicate predicate) {
  switch (predicate) {
  case CmpInst::FCMP_FALSE:
    comp_type = FALSE;
    loopTerminates = true;
    break;
  case CmpInst::FCMP_OEQ:
    comp_type = EQ;
    break;
  case CmpInst::FCMP_OGT:
    comp_type = GT;
    break;
  case CmpInst::FCMP_OGE:
    comp_type = GEQ;
    break;
  case CmpInst::FCMP_OLT:
    comp_type = LT;
    break;
  case CmpInst::FCMP_OLE:
    comp_type = LEQ;
    break;
  case CmpInst::FCMP_ONE:
    comp_type = NEQ;
    break;
  case CmpInst::FCMP_ORD:
    break;
  case CmpInst::FCMP_UNO:
    break;
  case CmpInst::FCMP_UEQ:
    comp_type = EQ;
    break;
  case CmpInst::FCMP_UGT:
    comp_type = GT;
    break;
  case CmpInst::FCMP_UGE:
    comp_type = GEQ;
    break;
  case CmpInst::FCMP_ULT:
    comp_type = LT;
    break;
  case CmpInst::FCMP_ULE:
    comp_type = LEQ;
    break;
  case CmpInst::FCMP_UNE:
    comp_type = NEQ;
    break;
  case CmpInst::FCMP_TRUE:
    comp_type = TRUE;
    loopTerminates = false;
    break;
  case CmpInst::BAD_FCMP_PREDICATE:
    break;
  case CmpInst::ICMP_EQ:
    comp_type = EQ;
    break;
  case CmpInst::ICMP_NE:
    comp_type = NEQ;
    break;
  case CmpInst::ICMP_UGT:
    comp_type = GT;
    break;
  case CmpInst::ICMP_UGE:
    comp_type = GEQ;
    break;
  case CmpInst::ICMP_ULT:
    comp_type = LT;
    break;
  case CmpInst::ICMP_ULE:
    comp_type = LEQ;
    break;
  case CmpInst::ICMP_SGT:
    comp_type = GT;
    break;
  case CmpInst::ICMP_SGE:
    comp_type = GEQ;
    break;
  case CmpInst::ICMP_SLT:
    comp_type = LT;
    break;
  case CmpInst::ICMP_SLE:
    comp_type = LEQ;

    break;
  case CmpInst::BAD_ICMP_PREDICATE:
    break;
  }
}