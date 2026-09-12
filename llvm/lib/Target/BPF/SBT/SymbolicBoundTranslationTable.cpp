//
// Created by deniz on 9/8/26.
//

#include "SymbolicBoundTranslationTable.h"

#include "SCEVCanonicalPrintVisitor.h"
#include "SBT/SBTFunctionArgEntry.h"
#include "SBT/SBTConstantEntry.h"
#include "SBT/SBTInstructionEntry.h"
#include "llvm/IR/Module.h"


#include "../../../include/llvm/CodeGen/PCSectionHelpers.h"

#define DEBUG_TYPE "SymbolicBoundTranslationTable"

SymbolicBoundTranslationTable::SymbolicBoundTranslationTable(ScalarEvolution &SE, Loop *loop, PHINode *iv, const SCEV *end, const SCEV *stride, uint64_t l_id) : SE(SE) {
  SCEVCanonicalPrintVisitor visitor(SE, loop, iv,  l_id);
  visitor.measure(end, stride);
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

SymbolicBoundTranslationTable::~SymbolicBoundTranslationTable() {
  LLVM_DEBUG(dbgs() << "translation table destructor called\n");
  for (auto *dep : dependencies) {
    delete dep;
  }
}

void SymbolicBoundTranslationTable::tagAndEmitTable(llvm::Function *function_to_tag, uint64_t loop_number) {
  LLVMContext &context = function_to_tag->getContext();

  auto loop_name = std::string("_loopdb_symbounds");
  const auto module_name = function_to_tag->begin()->getModule()->getModuleIdentifier();
  loop_name.insert(0, module_name);
  auto *old_mdnode = initOrGetPCSectionArrayFunction(context, function_to_tag, loop_name);
  LLVM_DEBUG(dbgs() << "Print mdnode data before: \n");
  LLVM_DEBUG(old_mdnode->printTree(dbgs()));
  LLVM_DEBUG(dbgs() << "\n");
  llvm::SmallVector<llvm::Constant *> entries;
  entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, loop_number)));
  //first, embed the string itself
  entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, this->getResultString().size())));
  entries.push_back(ConstantDataArray::getString(context, getResultString(), false));
  //now, we need to encode our table
  entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, this->dependencies.size())));
  auto dl = function_to_tag->begin()->getModule()->getDataLayout();
  for (auto dep : dependencies) {
    auto to_emit = dep->getTableEntry(context, dl);
    // https://stackoverflow.com/a/14608251
    // More specifically, https://stackoverflow.com/questions/14608250/how-can-i-find-the-size-of-a-type#comment125528828_14608251
    //uint64_t const_size = function_to_tag->begin()->getModule()->getDataLayout().getTypeAllocSize(to_emit->getType());
    //entries.push_back(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, const_size)));
    entries.push_back(to_emit);
  }
  function_to_tag->setMetadata("pcsections", appendToPCSectionArray(context, loop_name, old_mdnode, entries));
  LLVM_DEBUG(dbgs() << "Print mdnode data after: \n");
  LLVM_DEBUG(function_to_tag->getMetadata("pcsections")->printTree(dbgs()));
  LLVM_DEBUG(dbgs() << "\n");
}

std::string SymbolicBoundTranslationTable::getResultString() {
  return resultString;
}