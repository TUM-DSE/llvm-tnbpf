//
// Created by deniz on 7/13/26.
//

#include "../../include/llvm/CodeGen/PCSectionHelpers.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
using namespace llvm;

MDTuple *getPCSectionByName(MDNode *all_md, std::string &name) {
  //TODO: may have many arguments after each operand
  for (unsigned int i=0;i<all_md->getNumOperands();i += 2) {
    auto &label = all_md->getOperand(i);
    MDString *md_string = dyn_cast<MDString>(label.get());
    if (md_string->getString().compare(name) == 0) {
      //no additional logic needed
      MDTuple *entry = dyn_cast<MDTuple>(all_md->getOperand(i + 1).get());
      return entry;
    }
  }
  return nullptr;
}
void setPCSectionByName(MDNode *all_md, std::string &name, MDTuple *new_val) {
  //TODO: may have many arguments after each operand
  for (unsigned int i=0;i<all_md->getNumOperands();i += 2) {
    auto &label = all_md->getOperand(i);
    MDString *md_string = dyn_cast<MDString>(label.get());
    if (md_string->getString().compare(name) == 0) {
      //no additional logic needed
      all_md->replaceOperandWith(i + 1, new_val);
      return;
    }
  }
}
Constant *getPCSectionOperandAsConst(MDTuple *section, unsigned int i) {
  return dyn_cast<ConstantAsMetadata>(section->getOperand(i).get())->getValue();
}
ConstantInt *getPCSectionOperandAsIntConst(MDTuple *section, unsigned int i) {
  return dyn_cast<ConstantInt>(getPCSectionOperandAsConst(section, i));
}
Constant *embedU64(LLVMContext &context, uint64_t val) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(context), llvm::APInt(64, val));
}
Constant *embedU32(LLVMContext &context, uint32_t val) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(context), llvm::APInt(32, val));
}
Constant *embedU16(LLVMContext &context, uint16_t val) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt16Ty(context), llvm::APInt(16, val));
}
Constant *embedU8(LLVMContext &context, uint8_t val) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt8Ty(context), llvm::APInt(8, val));
}
Constant *embedU1(LLVMContext &context, bool val) {
  return llvm::Constant::getIntegerValue(llvm::Type::getInt1Ty(context), llvm::APInt(1, val));
}
MDNode *appendToPCSectionArray(LLVMContext &context, std::string name, MDNode *old_md, SmallVector<Constant *> entries) {

  auto relevant_section = getPCSectionByName(old_md, name);
  MDBuilder mb(context);

  ConstantAsMetadata *length = dyn_cast<ConstantAsMetadata>(relevant_section->getOperand(0));
  // I guess we have no option but to just rebuild the old constant from scratch
  auto &previous_metadata_count = dyn_cast<ConstantInt>(length->getValue())->getValue();
  const APInt &new_md_count = previous_metadata_count + 1;
  relevant_section->replaceOperandWith(0, ConstantAsMetadata::get(Constant::getIntegerValue(llvm::Type::getInt64Ty(context), new_md_count)));

  SmallVector<Metadata *> new_entries(relevant_section->getNumOperands() + entries.size());
  for (int i=0;i<relevant_section->getNumOperands();i++) {
    new_entries[i] = relevant_section->getOperand(i);
  }
  int offset = relevant_section->getNumOperands();
  for (int i=0;i<entries.size();i++) {
    new_entries[i+offset] = ConstantAsMetadata::get(entries[i]);
  }
  auto new_relevant_section = MDTuple::get(context, new_entries);
  setPCSectionByName(old_md, name, new_relevant_section);

  return old_md;
}

MDNode *initOrGetPCSectionArrayFunction(LLVMContext &ctx, Function *fun, std::string name) {
  MDBuilder mb(ctx);
  auto old_mdnode = fun->getMetadata("pcsections");
  if (!old_mdnode) {
    fun->setMetadata("pcsections", mb.createPCSections({
      {name, {Constant::getIntegerValue(llvm::Type::getInt64Ty(ctx), llvm::APInt(64, 0))}}
      }));
    old_mdnode = fun->getMetadata("pcsections");
  }

  if (!getPCSectionByName(old_mdnode, name)) {
    auto *new_mdnode = MDNode::concatenate(old_mdnode, mb.createPCSections({
      {name, {Constant::getIntegerValue(llvm::Type::getInt64Ty(ctx), llvm::APInt(64, 0))}}
      }));
    fun->setMetadata("pcsections", new_mdnode);
    old_mdnode = new_mdnode;
  }
  return old_mdnode;
}

MDNode *initOrGetPCSectionArrayInstruction(LLVMContext &ctx, Instruction *instr, std::string name) {
  MDBuilder mb(ctx);
  auto old_mdnode = instr->getMetadata("pcsections");
  if (!old_mdnode) {
    instr->setMetadata("pcsections", mb.createPCSections({
      {name, {Constant::getIntegerValue(llvm::Type::getInt64Ty(ctx), llvm::APInt(64, 0))}}
      }));
    old_mdnode = instr->getMetadata("pcsections");
  }

  if (!getPCSectionByName(old_mdnode, name)) {
    auto *new_mdnode = MDNode::concatenate(old_mdnode, mb.createPCSections({
      {name, {Constant::getIntegerValue(llvm::Type::getInt64Ty(ctx), llvm::APInt(64, 0))}}
      }));
    instr->setMetadata("pcsections", new_mdnode);
    old_mdnode = new_mdnode;
  }
  return old_mdnode;
}

