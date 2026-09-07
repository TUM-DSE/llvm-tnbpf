//
// Created by deniz on 7/13/26.
//

#include "BPFInstructionRegisterTaggingPass.h"
#define DEBUG_TYPE "bpf-instruction-register-tagging"
#include "../../../include/llvm/CodeGen/PCSectionHelpers.h"
#include "BPF.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"


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
            for (int i=0;i<pc_sections->getNumOperands();i+=2) {
              auto &name = pc_sections->getOperand(i);
              MDString *md_string = dyn_cast<MDString>(name.get());
              if (md_string->getString().compare(sym_db_name) == 0) {

                MDTuple *symdb_entry = dyn_cast<MDTuple>(pc_sections->getOperand(i + 1).get());

                LLVM_DEBUG(
                  dbgs() << "Instruction register tagging: old MDNode: \n Instr: ";
                  I.print(dbgs());
                  dbgs() << "\n Entry: ";
                  symdb_entry->printTree(dbgs());
                  dbgs() << "\n Entire Metadata: ";
                  pc_sections->printTree(dbgs());
                  dbgs() << "\n";

                );

                uint64_t opcode_llvm = dyn_cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(symdb_entry->getOperand(2).get())->getValue())->getLimitedValue(UINT32_MAX);
                LLVM_DEBUG(
                  dbgs() << "Instruction opcode: " << opcode_llvm << " plain name: " << Instruction::getOpcodeName(opcode_llvm) << "\n"
                  );
                bool remove_metadata = false;

                symdb_entry = MDTuple::get(LC, {symdb_entry->getOperand(0), symdb_entry->getOperand(1)});


                //TODO: this is a bodge, what if PCSections fully duplicates the instruction rather than emitting helpers?
                switch (opcode_llvm) {
                  case Instruction::TermOps::CondBr:
                    remove_metadata = !I.isConditionalBranch();
                    break;
                  case Instruction::TermOps::UncondBr:
                    remove_metadata = !I.isUnconditionalBranch();
                    break;
                  //TODO: Add more pruning steps if it's still broken

                  default:
                    break;
                }

                if (remove_metadata) {
                  // preserve all other PCSections, but wipe the entry for the instruction ID
                  LLVM_DEBUG(dbgs() << "Deattaching metadata from instruction...\n");

                  SmallVector<Metadata *, 10> new_pcsections = {};

                  for (int j=0;j<pc_sections->getNumOperands();j+=2) {
                    if (i == j) {
                      continue;
                    }
                    new_pcsections.push_back(pc_sections->getOperand(j).get());
                    new_pcsections.push_back(pc_sections->getOperand(j + 1).get());
                  }

                  if (new_pcsections.size() == 0) {
                    I.setPCSections(MF, nullptr);
                    LLVM_DEBUG(dbgs() << "No more PCSections!\n");
                  } else {
                    I.setPCSections(MF, MDTuple::get(LC, new_pcsections));
                    LLVM_DEBUG(
                        dbgs() << "Instruction register tagging: new MDNode: \n";
                        I.getPCSections()->printTree(dbgs());
                        dbgs() << "\n";
                      );
                  }




                  //continue to next instruction (TODO: this is horrible!!!)
                  goto post_instr;
                }
                //Remove the instruction opcode tagging

                SmallVector<Metadata *, 10> new_pcsections = {};

                for (int j=0;j<pc_sections->getNumOperands();j+=2) {
                  new_pcsections.push_back(pc_sections->getOperand(j).get());
                  if (i == j) {
                    new_pcsections.push_back(symdb_entry);
                  } else {
                    new_pcsections.push_back(pc_sections->getOperand(j + 1).get());
                  }

                }

                I.setPCSections(MF, MDTuple::get(LC, new_pcsections));

                LLVM_DEBUG(
                      dbgs() << "Instruction register tagging: new MDNode: \n";
                      I.getPCSections()->printTree(dbgs());
                      dbgs() << "\n";
                    );



                auto old_instr_register = dyn_cast<ConstantAsMetadata>(symdb_entry->getOperand(1).get());
                auto integer_const = dyn_cast<ConstantInt>(old_instr_register->getValue());
                if (I.getNumDefs() == 1) {
                  //TODO: probably a terrible way to do this but good enough for now
                  for (auto op : I.all_defs()) {
                    if (op.isDef()) {
                      //Better replacement code, less horrible duplication stuff:
                      SmallVector<Metadata *, 10> pcsections_register = {};

                      for (int j=0;j<pc_sections->getNumOperands();j+=2) {
                        pcsections_register.push_back(pc_sections->getOperand(j).get());
                        if (i == j) {
                          pcsections_register.push_back(symdb_entry);
                        } else {
                          pcsections_register.push_back(pc_sections->getOperand(j + 1).get());
                        }

                      }

                      I.setPCSections(MF, MDTuple::get(LC, pcsections_register));

                      symdb_entry->replaceOperandWith(1, ConstantAsMetadata::get(
                        llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(LC), llvm::APInt(32, op.getReg().id()))
                      ));


                      LLVM_DEBUG(
                        dbgs() << "Instruction register tagging: new MDNode: \n";
                        symdb_entry->printTree(dbgs());
                        dbgs() << "\n";
                      );

                      break;
                    }

                  }

                }
                }
            }
          }
          post_instr:
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