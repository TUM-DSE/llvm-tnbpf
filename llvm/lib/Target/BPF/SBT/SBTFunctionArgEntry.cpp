//
// Created by deniz on 9/8/26.
//

#include "SBTFunctionArgEntry.h"

llvm::Constant *SBTFunctionArgEntry::emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, arg->getArgNo()));
}
uint64_t SBTFunctionArgEntry::getEntryType() {
  return FunctionParameter;
}

SBTFunctionArgEntry::SBTFunctionArgEntry(llvm::Argument *arg) : arg(arg) {}

SBTFunctionArgEntry::~SBTFunctionArgEntry() {}