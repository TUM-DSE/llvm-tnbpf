//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_SYMBOLICBOUNDTRANSLATIONTABLE_H
#define LLVM_SYMBOLICBOUNDTRANSLATIONTABLE_H

#include "BPF.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "SymbolicBoundTranslationTableEntry.h"
using namespace llvm;

class SymbolicBoundTranslationTable {

    public:

    std::vector<Value *> original_values;
    SymbolicBoundTranslationTable(ScalarEvolution &SE, Loop *loop, PHINode *iv, const SCEV *end, const SCEV *stride, uint64_t l_id);

    void tagAndEmitTable(Function *function_to_tag, uint64_t loop_number);

    ~SymbolicBoundTranslationTable();

    std::string getResultString();

private:
  ScalarEvolution &SE;
  // TODO: I don't actually know if we need the * in here or if C++ sorts out ownership for us
  std::vector<SymbolicBoundTranslationTableEntry *> dependencies;
  std::string resultString;
};

#endif // LLVM_SYMBOLICBOUNDTRANSLATIONTABLE_H
