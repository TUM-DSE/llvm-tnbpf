//
// Created by deniz on 5/4/26.
//

#include "BPFSimplePass.h"
#include "BPF.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/MDBuilder.h"

#include <iostream>

#define DEBUG_TYPE "bpf-simple"

using namespace llvm;

namespace {
  struct BPFSimple : public MachineFunctionPass {
    static char ID;

    BPFSimple() : MachineFunctionPass(ID) {};

  public:

    bool runOnMachineFunction(MachineFunction &M) override {
      //LLVM_DEBUG(dbgs() << "begin bpf simple pass debug" << "\n");
      M.print(dbgs());
      auto &LC = M.getFunction().getContext();
      MDBuilder MB(LC);
      for (auto &x : M) {
        //LLVM_DEBUG(dbgs() << "Machine Basic Block: " << x << "\n");
        for (auto &y : x) {
          // https://stackoverflow.com/a/13816135
          if (y.isBranch()) {
            //y.setPCSections(M, MB.createPCSections({
            //  {"loop_info", {llvm::ConstantInt::get(LC, APInt(64, loop_count++))}}
            //}));
          }
          if (y.getPCSections()) {
              //do stuff with PC sectio
          }
          //LLVM_DEBUG(dbgs() << "Machine Instr: " << y << "\n");
        }
      }
      //LLVM_DEBUG(dbgs() << "end bpf simple pass debug" << "\n");
      return false;
    }

  };
}

INITIALIZE_PASS(BPFSimple, "bpf-simple",
                "BPF Print Instructions",
                false, false)

char BPFSimple::ID = 0;
FunctionPass* llvm::createBPFSimplePass() { return new BPFSimple(); }