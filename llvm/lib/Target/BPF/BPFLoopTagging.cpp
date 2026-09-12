//
// Created by deniz on 6/15/26.
//

#include "BPF.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/CodeGen/PCSectionHelpers.h"

#include "BPFPCSectionHelpers.h"
#include "BPFLoopTemplate.h"

#define DEBUG_TYPE "bpf-loop-tagging"

using namespace llvm;

namespace {


  struct BPFLoopTagging : LoopPass {
    static char ID;
    BPFLoopTagging() : LoopPass(ID) {}
    bool runOnLoop(Loop *L, LPPassManager &LPM) override {
      auto &context = L->getHeader()->getContext();
      LLVM_DEBUG(dbgs() << "begin bpf loop pass on loop:" << "\n");
      auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      //auto &loop_info = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
      LLVM_DEBUG(dbgs() << "got analysis" << "\n");

      BPFLoopTemplate loop_meta;



      bool loop_verification_failed = false;
      {
        SmallVector<BasicBlock *> loop_latches = {};
        L->getLoopLatches(loop_latches);
        for (auto BB : loop_latches) {
          loop_meta.latches.push_back(pcSectionGetInstructionID(BB->getTerminator()));
        }
      }

      auto lb = L->getBounds(SE);
      const SCEV *start_scev = nullptr;
      const SCEV *end_scev = nullptr;
      const SCEV *stride_scev = nullptr;

      if (auto *loop_guard = L->getLoopGuardBranch()) {
        loop_meta.br_id_tag = pcSectionGetInstructionID(loop_guard);
      }

      if (auto *cond_compare = L->getLatchCmpInst()) {
        loop_meta.cmp_id_tag = pcSectionGetInstructionID(cond_compare);
      }

      auto iv_val = L->getInductionVariable(SE);
      const SCEV *iv = nullptr;
      if (iv_val) {
        iv = SE.getSCEV(iv_val);
        LLVM_DEBUG(iv->print(dbgs()));
        LLVM_DEBUG(dbgs() << "\n");

      }

      if (lb.has_value()) {
        auto unpacked_lb = lb.value();

        auto &initial = unpacked_lb.getInitialIVValue();
        start_scev = SE.getSCEV(&initial);
        auto &end = unpacked_lb.getFinalIVValue();
        end_scev = SE.getSCEV(&end);
        auto step = unpacked_lb.getStepValue();

        stride_scev = SE.getSCEV(step);
        LLVM_DEBUG(dbgs() << "stride scev: ");
        LLVM_DEBUG(stride_scev->print(dbgs()));
        LLVM_DEBUG(dbgs() << "\n");

        switch (unpacked_lb.getDirection()) {
        case Loop::LoopBounds::Direction::Increasing:
          LLVM_DEBUG(dbgs() << "loop increasing" << "\n");
          loop_meta.loopIsAscending = true;

          break;
        case Loop::LoopBounds::Direction::Unknown:
          LLVM_DEBUG(dbgs() << "loop unknown" << "\n");
          break;
        case Loop::LoopBounds::Direction::Decreasing:
          LLVM_DEBUG(dbgs() << "loop decreasing" << "\n");
          loop_meta.loopIsAscending = false;

          break;
        }


        if (!L->isLoopInvariant(&end)) {
          loop_verification_failed = true;
          LLVM_DEBUG(dbgs() << "Loop verification failed: end not invariant\n");
        }
        if (!L->isLoopInvariant(step)) {
          loop_verification_failed = true;
          LLVM_DEBUG(dbgs() << "Loop verification failed: stride not invariant\n");
        }

        //TODO: Maybe also get info about the initial, end and stride values as string SCEVs and pack them in
        //with the same architecture?
        if (ConstantInt *init_int = dyn_cast<ConstantInt>(&initial)) {
          LLVM_DEBUG(dbgs() << "print initial as constant int:" << "\n");
          LLVM_DEBUG(dbgs() << init_int->getValue());
          //TODO: what if we overflow / underflow?
          loop_meta.start = init_int->getSExtValue();
        }


        if (ConstantInt *final_int = dyn_cast<ConstantInt>(&end)) {
          //TODO: what if we overflow / underflow?
          loop_meta.finish = final_int->getSExtValue();
        }
        if (ConstantInt *step_int = dyn_cast<ConstantInt>(step)) {
          //TODO: what if we overflow / underflow?
          loop_meta.stride = step_int->getSExtValue();
        }


        loop_meta.importLoopComparison(unpacked_lb.getCanonicalPredicate());
      if (iv_val) {
        SymbolicBoundTranslationTable *sym_max_measure_standardized = new SymbolicBoundTranslationTable(SE, L, iv_val, end_scev, stride_scev, loop_meta.loop_number);
        loop_meta.translation_table_measure = sym_max_measure_standardized;
        LLVM_DEBUG(dbgs() << "symbolic max measure SCEV result string: " << sym_max_measure_standardized->getResultString() << "\n");
        LLVM_DEBUG(dbgs() << "results vector:\n");
        for (int i=0;i<loop_meta.translation_table_measure.value()->original_values.size();i++) {
          LLVM_DEBUG(dbgs() << "%" << i << " -> ");
          LLVM_DEBUG(loop_meta.translation_table_measure.value()->original_values[i]->printAsOperand(dbgs()));
          LLVM_DEBUG(dbgs() << "\n");
        }
      }
      LLVM_DEBUG(dbgs() << "end bpf loop pass on loop:" << "\n");

      } else {
        LLVM_DEBUG(dbgs() << "failed to get loop bounds" << "\n");
        //Time to add some handler code here
        //We can do much less in this case
      }
      loop_meta.pcSectionLoopClassifyTag(L->getHeader()->getParent());
      LLVM_DEBUG(dbgs() << "print entire loop:" << "\n");
      printLoop(*L, dbgs());
      if (isFinite(L)) {
        LLVM_DEBUG(dbgs() << "llvm says the loop is finite\n");
      } else {
        LLVM_DEBUG(dbgs() << "llvm does not say the loop is finite\n");
      }

      return false;
    }
    void getAnalysisUsage(AnalysisUsage &Info) const override {
        //TODO: There are two analysis passes called "Machine Loop Info" and "Loop Info"
        // I don't know which level to be working on
        //Info.addRequiredTransitive<LoopInfoWrapperPass>();
      Info.addRequiredTransitive<ScalarEvolutionWrapperPass>();
      getLoopAnalysisUsage(Info);
    }
  };
}

INITIALIZE_PASS_BEGIN(BPFLoopTagging, DEBUG_TYPE, "Tags loops with pc section annotations for later proof generation", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopPass)
INITIALIZE_PASS_END(BPFLoopTagging, DEBUG_TYPE, "Tags loops with pc section annotations for later proof generation", false, false)
char BPFLoopTagging::ID = 0;
LoopPass *llvm::createBPFLoopTaggingPass() {
  return new BPFLoopTagging();
}
