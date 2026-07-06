//
// Created by deniz on 7/6/26.
//
#include "llvm/IR/DerivedTypes.h"
#include "BPFTypeEmbedding.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/TypedPointerType.h"
#include "llvm/IR/Constants.h"
#include "BPF.h"

using namespace llvm;
static inline llvm::Constant *intValue(llvm::LLVMContext &ctx, uint64_t val) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(ctx), llvm::APInt(64, val));
}
llvm::Constant *embedType(llvm::LLVMContext &ctx, llvm::DataLayout &dl, llvm::Type *type) {
  std::vector<Constant *> entries;
  entries.push_back(intValue(ctx, type->getTypeID()));

  switch (type->getTypeID()) {

  case Type::IntegerTyID: {
    auto i_cast = dyn_cast<IntegerType>(type);
    entries.push_back(intValue(ctx, i_cast->getBitWidth()));
    break;
  }
  case Type::ByteTyID: {
    auto b_cast = dyn_cast<ByteType>(type);
    entries.push_back(intValue(ctx, b_cast->getBitWidth()));
    break;
  }
  case Type::FunctionTyID: {
    auto fun_cast = dyn_cast<FunctionType>(type);
    entries.push_back(intValue(ctx, fun_cast->isVarArg()));
    entries.push_back(embedType(ctx, dl, fun_cast->getReturnType()));
    entries.push_back(intValue(ctx, fun_cast->getNumParams()));
    for (auto param : fun_cast->params()) {
      entries.push_back(embedType(ctx, dl, param));
    }
    break;
  }
  case Type::StructTyID: {
    auto str_cast = dyn_cast<StructType>(type);
    entries.push_back(intValue(ctx, str_cast->isPacked()));
    entries.push_back(intValue(ctx, str_cast->isOpaque()));
    if (!str_cast->isOpaque()) {
      entries.push_back(intValue(ctx, str_cast->getNumElements()));
      for (auto elem : str_cast->elements()) {
        entries.push_back(embedType(ctx, dl, elem));
      }
    }
    break;
  }
  case Type::ArrayTyID: {
    auto *arr_cast = dyn_cast<ArrayType>(type);
    entries.push_back(intValue(ctx, arr_cast->getNumElements()));
    entries.push_back(embedType(ctx, dl, arr_cast->getElementType()));
    break;
  }
  case Type::FixedVectorTyID: {
    auto *fv_cast = dyn_cast<FixedVectorType>(type);
    entries.push_back(intValue(ctx, fv_cast->getElementCount().getFixedValue()));
    entries.push_back(embedType(ctx, dl, fv_cast->getElementType()));
    break;
  }
  case Type::ScalableVectorTyID: {
    auto *sv_cast = dyn_cast<ScalableVectorType>(type);
    entries.push_back(embedType(ctx, dl, sv_cast->getElementType()));
    break;
  }
  case Type::TypedPointerTyID: {
    auto *tp_cast = dyn_cast<llvm::TypedPointerType>(type);
    entries.push_back(embedType(ctx, dl, tp_cast->getElementType()));
    break;
  }
  default:
    break;
  }

  return llvm::ConstantStruct::getAnon(entries, true);
}