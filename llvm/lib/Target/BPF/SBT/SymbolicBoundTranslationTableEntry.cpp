//
// Created by deniz on 9/8/26.
//

#include "SymbolicBoundTranslationTableEntry.h"
#include "llvm/IR/Constants.h"
llvm::Constant *SymbolicBoundTranslationTableEntry::getTableEntry(llvm::LLVMContext &context, llvm::DataLayout &dl) {
  return llvm::ConstantStruct::getAnon({
    llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, getEntryType())),
    emitToTable(context, dl)
  }, true);
}

SymbolicBoundTranslationTableEntry::~SymbolicBoundTranslationTableEntry() {}