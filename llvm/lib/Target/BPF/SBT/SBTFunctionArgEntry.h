//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_SBTFUNCTIONARGENTRY_H
#define LLVM_SBTFUNCTIONARGENTRY_H

#include "SymbolicBoundTranslationTableEntry.h"

class SBTFunctionArgEntry : public SymbolicBoundTranslationTableEntry {
  llvm::Argument *arg;
  llvm::Constant *emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl) override;
  uint64_t getEntryType() override;
public:
  SBTFunctionArgEntry(llvm::Argument *arg);
  ~SBTFunctionArgEntry();
};

#endif // LLVM_SBTFUNCTIONARGENTRY_H
