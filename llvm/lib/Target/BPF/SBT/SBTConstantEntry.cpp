//
// Created by deniz on 9/8/26.
//

#include "SBTConstantEntry.h"
#include "llvm/IR/Constants.h"

llvm::Constant * SBTConstantEntry::emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl) {
  //TODO: if c is a global constant value, this just copies the whole entire global constant to c
  return llvm::ConstantStruct::getAnon({
    llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, dl.getTypeAllocSize(c->getType()))),
    //TODO: no idea how to handle embedding non integer types
    c
  }, true);
}

uint64_t SBTConstantEntry::getEntryType() {
  return Constant;
}

SBTConstantEntry::SBTConstantEntry(llvm::Constant *c) : c(c) {}
SBTConstantEntry::~SBTConstantEntry() {}