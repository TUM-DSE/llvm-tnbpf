//
// Created by deniz on 7/13/26.
//

#include "BPFInstructionRegisterTaggingPass.h"
#define DEBUG_TYPE "bpf-instruction-register-tagging"
#include "BPF.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
using namespace llvm;

namespace {
  struct BPFInstructionRegisterTagging : public MachineFunctionPass {
    static char ID;
    BPFInstructionRegisterTagging() : MachineFunctionPass(ID) {};

  public:
    bool runOnMachineFunction(MachineFunction &MF) override {
      LLVM_DEBUG(dbgs() << "begin BPF instruction register tagging pass\n");
      auto &LC = MF.getFunction().getContext();
      auto module = MF.getFunction().begin()->getModule();
      MachineRegisterInfo reg_info(&MF);

      auto sym_db_name = std::string("_sym_instr_db");
      const auto module_name = module->getModuleIdentifier();
      sym_db_name.insert(0, module_name);

      MDBuilder MB(LC);
      for (auto &BB : MF) {
        for (auto &I : BB) {
          auto pc_sections = I.getPCSections();
          if (pc_sections) {
            LLVM_DEBUG(dbgs() << "total operand count before: " << pc_sections->getNumOperands() << "\n");
            LLVM_DEBUG(pc_sections->printTree(dbgs(), module));
            for (int i=0;i<pc_sections->getNumOperands();i+=2) {
              auto &name = pc_sections->getOperand(i);
              MDString *md_string = dyn_cast<MDString>(name.get());
              if (md_string->getString().compare(sym_db_name) == 0) {
                LLVM_DEBUG(dbgs() << "found symdb entry\n");

                MDTuple *symdb_entry = dyn_cast<MDTuple>(pc_sections->getOperand(i + 1).get());
                LLVM_DEBUG(dbgs() << "operand count before: " << symdb_entry->getNumOperands() << "\n");
                auto old_instr_register = dyn_cast<ConstantAsMetadata>(symdb_entry->getOperand(1).get());
                auto integer_const = dyn_cast<ConstantInt>(old_instr_register->getValue());
                LLVM_DEBUG(dbgs() << "old integer value: " << *integer_const->getValue().getRawData() << "\n");
                if (I.getNumDefs() == 1) {
                  //TODO: probably a terrible way to do this but good enough for now
                  for (auto op : I.all_defs()) {
                    if (op.isDef()) {
                      symdb_entry->replaceOperandWith(1, ConstantAsMetadata::get(
                        llvm::Constant::getIntegerValue(llvm::Type::getInt64Ty(LC), llvm::APInt(64, op.getReg().id()))
                      ));

                      break;
                    }

                  }

                }
                }

            }
            for (int i=0;i<pc_sections->getNumOperands();i+=2) {
              auto &name = pc_sections->getOperand(i);
              MDString *md_string = dyn_cast<MDString>(name.get());
              if (md_string->getString().compare(sym_db_name) == 0) {
                LLVM_DEBUG(dbgs() << "found symdb entry again\n");
                MDTuple *symdb_entry = dyn_cast<MDTuple>(pc_sections->getOperand(i + 1).get());
                auto old_instr_register = dyn_cast<ConstantAsMetadata>(symdb_entry->getOperand(1).get());
                auto integer_const = dyn_cast<ConstantInt>(old_instr_register->getValue());
                LLVM_DEBUG(dbgs() << "new integer value: " << *integer_const->getValue().getRawData() << "\n");
                LLVM_DEBUG(dbgs() << "operand count after: " << symdb_entry->getNumOperands() << "\n");
              }
            }
            LLVM_DEBUG(dbgs() << "total operand count after: " << pc_sections->getNumOperands() << "\n");
            LLVM_DEBUG(pc_sections->printTree(dbgs(), module));
          }
        }
      }
      LLVM_DEBUG(dbgs() << "end BPF instruction register tagging pass\n");
      return false;
    }

  };
};

INITIALIZE_PASS(BPFInstructionRegisterTagging, DEBUG_TYPE, "BPF Instruction Register Tagging", false, false)
char BPFInstructionRegisterTagging::ID = 0;
FunctionPass *llvm::createBPFInstructionRegisterTaggingPass() { return new BPFInstructionRegisterTagging(); }