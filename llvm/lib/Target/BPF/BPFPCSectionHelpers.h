//
// Created by deniz on 9/8/26.
//

#ifndef LLVM_BPFPCSECTIONHELPERS_H
#define LLVM_BPFPCSECTIONHELPERS_H

#include "BPF.h"

llvm::Constant *pcSectionGetInstructionID(llvm::Instruction *instr);

#endif // LLVM_BPFPCSECTIONHELPERS_H