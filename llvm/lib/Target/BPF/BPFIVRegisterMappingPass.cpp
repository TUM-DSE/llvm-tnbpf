//
// Created by deniz on 8/25/26.
//

#include "BPFIVRegisterMappingPass.h"
#include "../../../include/llvm/CodeGen/PCSectionHelpers.h"
#include "BPF.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/VirtRegMap.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"

#define DEBUG_TYPE "bpf-iv-register-mapping"

using namespace llvm;

namespace {
  struct BPFIVRegisterMapping : public MachineFunctionPass {
    static char ID;

    BPFIVRegisterMapping() : MachineFunctionPass(ID) {};

    bool runOnMachineFunction(MachineFunction &MF) override {
      LLVM_DEBUG(dbgs() << "IV Register mapping pass runs\n");

      VirtRegMap &VRM = getAnalysis<VirtRegMapWrapperLegacy>().getVRM();

      auto &LC = MF.getFunction().getContext();
      auto module = MF.getFunction().begin()->getModule();
      MachineRegisterInfo reg_info(&MF);

      const auto module_name = module->getModuleIdentifier();
      auto bpf_tag_name = std::string("_iv_db");
      bpf_tag_name.insert(0, module_name);

      MDBuilder MB(LC);

      auto pc_sections = MF.getFunction().getMetadata("pcsections");
      if (pc_sections) {
        for (int i=0;i<pc_sections->getNumOperands();i+=2) {
          auto &name = pc_sections->getOperand(i);
          MDString *md_string = dyn_cast<MDString>(name.get());
          if (md_string->getString().compare(bpf_tag_name) == 0) {
            MDTuple *bpf_db_entry = dyn_cast<MDTuple>(pc_sections->getOperand(i + 1).get());
            LLVM_DEBUG(dbgs() << "BPF IV Register PCSection: ");
            LLVM_DEBUG(bpf_db_entry->printTree(dbgs()));
            LLVM_DEBUG(dbgs() << "\n");
            uint64_t section_count =  dyn_cast<ConstantInt>(
              dyn_cast<ConstantAsMetadata>(
                bpf_db_entry
                  ->getOperand(0).get()
                  )
                  ->getValue()
                  )
              ->getValue()
              .getLimitedValue(UINT64_MAX);

            for (uint64_t section_no = 0; section_no < section_count; section_no++) {
              auto old_instr_register = dyn_cast<ConstantAsMetadata>(bpf_db_entry->getOperand(section_no * 2 + 2).get());
              auto integer_const = dyn_cast<ConstantInt>(old_instr_register->getValue());

              Register virt_reg(integer_const->getLimitedValue(UINT32_MAX));
              Register phys_reg = virt_reg.isVirtual() ? VRM.getPhys(virt_reg) : virt_reg.asMCReg();
              bpf_db_entry->replaceOperandWith(section_no * 2 + 2, ConstantAsMetadata::get(embedU32(LC, phys_reg.id())));
            }
            LLVM_DEBUG(dbgs() << "BPF IV Register PCSection - post-assignment: ");
            LLVM_DEBUG(bpf_db_entry->printTree(dbgs()));
            LLVM_DEBUG(dbgs() << "\n");
          }
        }
      }

      return false;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      MachineFunctionPass::getAnalysisUsage(AU);
      AU.addRequired<VirtRegMapWrapperLegacy>();
      AU.setPreservesAll();
    }


  };
}

INITIALIZE_PASS(BPFIVRegisterMapping, DEBUG_TYPE, "BPF Obtain the physical register that the IV is mapped in", false, false)

char BPFIVRegisterMapping::ID = 0;
FunctionPass *llvm::createBPFIVRegisterMappingPass() {
  return new BPFIVRegisterMapping();
}
