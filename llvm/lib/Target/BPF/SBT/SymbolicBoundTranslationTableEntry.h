//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_SYMBOLICBOUNDTRANSLATIONTABLEENTRY_H
#define LLVM_SYMBOLICBOUNDTRANSLATIONTABLEENTRY_H

#include "../BPF.h"

class SymbolicBoundTranslationTableEntry {
  virtual llvm::Constant *emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl);
  virtual uint64_t getEntryType();
public:
  enum EntryTypes {
    Instruction,
    Constant,
    FunctionParameter
  };

  llvm::Constant *getTableEntry(llvm::LLVMContext &context, llvm::DataLayout &dl);
  virtual ~SymbolicBoundTranslationTableEntry();
};

#endif // LLVM_SYMBOLICBOUNDTRANSLATIONTABLEENTRY_H
