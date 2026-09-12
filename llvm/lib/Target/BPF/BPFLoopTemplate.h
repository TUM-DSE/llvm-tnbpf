//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_BPFLOOPTEMPLATE_H
#define LLVM_BPFLOOPTEMPLATE_H

#include "cstdint"
#include "optional"
#include "SBT/SymbolicBoundTranslationTable.h"
#include "BPF.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"


#define DEBUG_TYPE "BPFLoopTemplate"

using namespace llvm;

class BPFLoopTemplate {
  public:
  uint64_t loop_number;
  std::optional<int64_t> start;
  std::optional<int64_t> finish;
  std::optional<int64_t> stride;
  enum ComparisonType {
    //Bitmasking enabled comparison enum
    //        < = >
    FALSE,  //0 0 0
    GT,     //0 0 1
    EQ,     //0 1 0
    GEQ,    //0 1 1
    LT,     //1 0 0
    NEQ,    //1 0 1
    LEQ,    //1 1 0
    TRUE,   //1 1 1
    C_UNK,
  };
  ComparisonType comp_type;
  std::optional<bool> loopIsAscending;
  std::optional<bool> loopTerminates;
  //TODO: populate these two as well
  std::optional<uint64_t> exact_exit_count;
  std::optional<uint64_t> constant_max_exit_count;

  std::optional<SymbolicBoundTranslationTable *> translation_table_measure;
  SmallVector<Constant *> latches;
  std::optional<Constant *> cmp_id_tag;
  std::optional<Constant *> br_id_tag;
  BPFLoopTemplate();
  ~BPFLoopTemplate();
  void pcSectionLoopClassifyTag(Function *func);
  void importLoopComparison(ICmpInst::Predicate predicate);
};

#endif // LLVM_BPFLOOPTEMPLATE_H
