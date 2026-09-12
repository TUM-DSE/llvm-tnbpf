//
// Created by deniz on 9/8/26.
//

#include "BPFPCSectionHelpers.h"
#include "atomic"

#include "../../../include/llvm/CodeGen/PCSectionHelpers.h"

#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"

#define DEBUG_TYPE "BPFPCSectionHelpers"

using namespace llvm;

std::atomic<unsigned long long> sym_ref_instruction_id = 0;

llvm::Constant *pcSectionGetInstructionID(llvm::Instruction *instr) {
  llvm::LLVMContext &context = instr->getContext();
  llvm::MDBuilder mb(context);
  auto loop_name = std::string("_sym_instr_db");
  const auto module_name = instr->getModule()->getModuleIdentifier();
  loop_name.insert(0, module_name);
  auto old_mdnode = instr->getMetadata("pcsections");
  if (old_mdnode) {
    //TODO: redundant section search logic, move elsewhere and turn into a getPCSectionByName helper function somewhere?
    if (auto symdb_entry = getPCSectionByName(old_mdnode, loop_name)) {
      auto old_instr_tag_id = dyn_cast<ConstantAsMetadata>(symdb_entry->getOperand(0).get());
      return old_instr_tag_id->getValue();
    }
  }

  uint64_t inst_tag_id = sym_ref_instruction_id.fetch_add(1);
  llvm::MDNode *node = mb.createPCSections({
        {loop_name, {
          llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, inst_tag_id)),
          //Instruction type goes here
          llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(context), llvm::APInt(32, instr->getOpcode()))
        }}
  });


  LLVM_DEBUG(
    dbgs() << "Instruction tagging: new MDNode: \n";
    node->printTree(dbgs());
    dbgs() << "\n on instr: ";
    instr->print(dbgs());
    dbgs() << "\n";
  );
  if (old_mdnode) {
    LLVM_DEBUG(
    dbgs() << "Instruction tagging: old MDNode: \n";
    old_mdnode->printTree(dbgs());
    dbgs() << "\n";
  );
    node = llvm::MDNode::concatenate(old_mdnode, node);
    LLVM_DEBUG(
    dbgs() << "Instruction tagging: total MDNode: \n";
    node->printTree(dbgs());
    dbgs() << "\n";
  );
  }
  instr->setMetadata("pcsections", node);
  return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, inst_tag_id));
}