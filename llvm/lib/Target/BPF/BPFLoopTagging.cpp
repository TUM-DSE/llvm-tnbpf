//
// Created by deniz on 6/15/26.
//

#include "BPF.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/InitializePasses.h"
#define DEBUG_TYPE "bpf-loop-tagging"

using namespace llvm;

namespace {
  struct BPFLoopTagging : public LoopPass {
    static char ID;
    BPFLoopTagging() : LoopPass(ID) {}
  public:
    bool runOnLoop(Loop *L, LPPassManager &LPM) override {
      LLVM_DEBUG(dbgs() << "begin bpf loop pass on loop:" << "\n");
      auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      auto &loop_info = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
      LLVM_DEBUG(dbgs() << "got analysis" << "\n");

      LLVM_DEBUG(dbgs() << "print entire loop:" << "\n");
      printLoop(*L, dbgs());
      auto lb = L->getBounds(SE);
      if (lb.has_value()) {
        auto unpacked_lb = lb.value();
        switch (unpacked_lb.getDirection()) {
        case Loop::LoopBounds::Direction::Increasing:
          LLVM_DEBUG(dbgs() << "loop increasing" << "\n");
          break;
        case Loop::LoopBounds::Direction::Unknown:
          LLVM_DEBUG(dbgs() << "loop unknown" << "\n");
          break;
        case Loop::LoopBounds::Direction::Decreasing:
          LLVM_DEBUG(dbgs() << "loop decreasing" << "\n");
          break;
        }
        auto &initial = unpacked_lb.getFinalIVValue();
        auto initial_scev = SE.getSCEV(&initial);
        if (ConstantInt *init_int = dyn_cast<ConstantInt>(&initial)) {
          LLVM_DEBUG(dbgs() << "print initial as constant int:" << "\n");
          LLVM_DEBUG(dbgs() << init_int->getValue());
        }

      } else {
        LLVM_DEBUG(dbgs() << "failed to get loop bounds" << "\n");
      }
      LLVM_DEBUG(dbgs() << "end bpf loop pass on loop:" << "\n");
      return false;
    }
    void getAnalysisUsage(AnalysisUsage &Info) const override {
        //TODO: There are two analysis passes called "Machine Loop Info" and "Loop Info"
        // I don't know which level to be working on
        //Info.addRequiredTransitive<LoopInfoWrapperPass>();
        //Info.addRequiredTransitive<ScalarEvolutionWrapperPass>();
      getLoopAnalysisUsage(Info);
    }
  };
}

INITIALIZE_PASS_BEGIN(BPFLoopTagging, "bpf-loop-tagging", "Tags loops with pc section annotations for later proof generation", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopPass)
INITIALIZE_PASS_END(BPFLoopTagging, "bpf-loop-tagging", "Tags loops with pc section annotations for later proof generation", false, false)
char BPFLoopTagging::ID = 0;
LoopPass *llvm::createBPFLoopTaggingPass() {
  return new BPFLoopTagging();
}
