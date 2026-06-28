//
// Created by deniz on 6/23/26.
//

#ifndef LLVM_SCEVCANONICALPRINTVISITOR_H
#define LLVM_SCEVCANONICALPRINTVISITOR_H
#include "../../../include/llvm/ADT/DenseMap.h"
#include "../../../include/llvm/Analysis/ScalarEvolutionExpressions.h"
#include "BPF.h"

namespace llvm {

class SCEVCanonicalPrintVisitor {
protected:
  std::vector<Value *> Result;
  std::string StringResult;
  llvm::raw_string_ostream OS;

private:
  void printValue(Value *val);

public:
  SCEVCanonicalPrintVisitor();
  void visit(const SCEV *S);
  const std::pair<std::vector<Value *>, std::string> collectResults();
  //TODO: test this function somehow, idk
  void printPhi(PHINode *phi);
};

} // namespace llvm

#endif // LLVM_SCEVCANONICALPRINTVISITOR_H
