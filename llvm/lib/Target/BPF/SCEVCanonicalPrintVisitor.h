//
// Created by deniz on 6/23/26.
//

#ifndef LLVM_SCEVCANONICALPRINTVISITOR_H
#define LLVM_SCEVCANONICALPRINTVISITOR_H
#include "../../../include/llvm/ADT/DenseMap.h"
#include "../../../include/llvm/Analysis/ScalarEvolutionExpressions.h"
#include "BPF.h"

#include "llvm/Analysis/LoopInfo.h"

namespace llvm {

class SCEVCanonicalPrintVisitor {
protected:
  std::vector<Value *> Result;
  std::string StringResult;
  llvm::raw_string_ostream OS;
  PHINode *iv;
  Loop *loop;
  ScalarEvolution &SE;

private:
  void printValue(Value *val);
  void visit(const SCEV *S);
  bool iv_being_bootstrapped;
  std::string iv_string;
  std::string iter_no_string;

public:
  SCEVCanonicalPrintVisitor(ScalarEvolution &SE, Loop *loop, PHINode *iv);
  void measure(const SCEV *start, const SCEV *end, const SCEV *stride);
  const std::pair<std::vector<Value *>, std::string> collectResults();
  //TODO: test this function somehow, idk
  void printPhi(PHINode *phi);
};

} // namespace llvm

#endif // LLVM_SCEVCANONICALPRINTVISITOR_H
