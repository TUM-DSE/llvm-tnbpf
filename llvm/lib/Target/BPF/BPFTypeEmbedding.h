//
// Created by deniz on 7/6/26.
//

#ifndef LLVM_BPFTYPEEMBEDDING_H
#define LLVM_BPFTYPEEMBEDDING_H
#include "llvm/IR/Constant.h"
llvm::Constant *embedType(llvm::LLVMContext &ctx, llvm::DataLayout &dl, llvm::Type *type);

#endif // LLVM_BPFTYPEEMBEDDING_H
