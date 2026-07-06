//
// Created by deniz on 7/6/26.
//
#include "BPFFunctionTagging.h"
#include "BPF.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/Constants.h"

#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "BPFTypeEmbedding.h"
#define DEBUG_TYPE "bpf-function-tagging"

using namespace llvm;

namespace {
  struct BPFFunctionTagging : public FunctionPass {
    static char ID;
    BPFFunctionTagging() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override {
      auto section_name = std::string("_functiondb");
      const auto module_name = F.begin()->getModule()->getModuleIdentifier();
      section_name.insert(0, module_name);
      auto old_mdnode = F.getMetadata("pcsections");
      MDBuilder mb(F.getContext());
      auto dl = F.begin()->getModule()->getDataLayout();
      llvm::MDNode *node = mb.createPCSections({
        {section_name, {embedType(F.getContext(), dl, F.getFunctionType())}}}
        );

      if (old_mdnode) {
        node = llvm::MDNode::concatenate(old_mdnode, node);
      }
      F.setMetadata("pcsections", node);

      return false;
    }
  };
}

INITIALIZE_PASS(BPFFunctionTagging, DEBUG_TYPE,
                "BPF Tag Functions",
                false, false)

char BPFFunctionTagging::ID = 0;
FunctionPass* llvm::createBPFFunctionTaggingPass() { return new BPFFunctionTagging(); }
