//
// Created by deniz on 7/13/26.
//

#ifndef LLVM_BPFPCSECTIONHELPERS_H
#define LLVM_BPFPCSECTIONHELPERS_H
#include <string>

#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"

using namespace llvm;
MDTuple *getPCSectionByName(MDNode *all_md, std::string &name);
Constant *getPCSectionOperandAsConst(MDTuple *section, unsigned int i);
ConstantInt *getPCSectionOperandAsIntConst(MDTuple *section, unsigned int i);
Constant *embedU64(LLVMContext &context, uint64_t val);
Constant *embedU32(LLVMContext &context, uint32_t val);
Constant *embedU16(LLVMContext &context, uint16_t val);
Constant *embedU8(LLVMContext &context, uint8_t val);
Constant *embedU1(LLVMContext &context, bool val);
MDNode *appendToPCSectionArray(LLVMContext &context, std::string name, MDNode *old_md, SmallVector<Constant *> entries);
MDNode *initOrGetPCSectionArrayFunction(LLVMContext &ctx, Function *fun, std::string name);
MDNode *initOrGetPCSectionArrayInstruction(LLVMContext &ctx, Instruction *instr, std::string name);

#endif // LLVM_BPFPCSECTIONHELPERS_H
