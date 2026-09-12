//
// Created by deniz on 9/8/26.
//

#include "SBTInstructionEntry.h"
#include "../BPFPCSectionHelpers.h"

llvm::Constant * SBTInstructionEntry::emitToTable(llvm::LLVMContext &context, llvm::DataLayout &dl) {
  return pcSectionGetInstructionID(inst);
}

uint64_t SBTInstructionEntry::getEntryType() {
  return Instruction;
}

SBTInstructionEntry::SBTInstructionEntry(llvm::Instruction *inst) : inst(inst) {}
SBTInstructionEntry::~SBTInstructionEntry() {}