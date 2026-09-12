//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_SBTINSTRUCTIONENTRY_H
#define LLVM_SBTINSTRUCTIONENTRY_H

#include "SymbolicBoundTranslationTableEntry.h"

class SBTInstructionEntry : public SymbolicBoundTranslationTableEntry {

  llvm::Instruction *inst;
  llvm::Constant *emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl) override;
  uint64_t getEntryType() override;
public:

  SBTInstructionEntry(llvm::Instruction *inst);

  ~SBTInstructionEntry() override;
};

#endif // LLVM_SBTINSTRUCTIONENTRY_H
