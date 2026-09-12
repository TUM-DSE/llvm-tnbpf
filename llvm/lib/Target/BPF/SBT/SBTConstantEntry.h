//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_SBTCONSTANTENTRY_H
#define LLVM_SBTCONSTANTENTRY_H

#include "SymbolicBoundTranslationTableEntry.h"

class SBTConstantEntry : public SymbolicBoundTranslationTableEntry {
  llvm::Constant *c;
  llvm::Constant *emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl) override;
  uint64_t getEntryType() override;
public:
  SBTConstantEntry(llvm::Constant *c);
  ~SBTConstantEntry() override;
};

#endif // LLVM_SBTCONSTANTENTRY_H
