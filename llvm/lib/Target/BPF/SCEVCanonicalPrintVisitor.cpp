//
// Created by deniz on 6/23/26.
//

#include "SCEVCanonicalPrintVisitor.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/InterleavedRange.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "BPFPCSectionHelpers.h"

#define DEBUG_TYPE "llvm_bpf_scev_printer"

namespace llvm {

void SCEVCanonicalPrintVisitor::measure(const SCEV *end,
                                        const SCEV *stride) {
  /*
  case Loop::LoopBounds::Direction::Increasing:
  measure_scev = SE.getUDivCeilSCEV(SE.getMinusSCEV(end_scev, iv), stride_scev);
  break;

  case Loop::LoopBounds::Direction::Unknown:
  break;
  */
  auto iv_scev = SE.getUnknown(iv);
  if (!SE.isLoopInvariant(stride, this->loop) || !SE.isLoopInvariant(end, this->loop)) {
    OS << "*** ERROR - STRIDE OR END NOT LOOP INVARIANT ***";
    return;
  }
  const SCEV *measure;
  if (isKnownNegativeInLoop(stride, this->loop, SE)) {
    measure = SE.getMinusSCEV(iv_scev, end);
  } else {
    measure = SE.getMinusSCEV(end, iv_scev);
  }

  auto bpf_tag_name = std::string("_BPF_internal_phi_linkage");
  auto module = iv->getModule();
  const auto module_name = module->getModuleIdentifier();
  bpf_tag_name.insert(0, module_name);

  //tag iv with pcSection
  MDNode *old_md = initOrGetPCSectionArrayInstruction(iv->getContext(), iv, bpf_tag_name);
  MDNode *new_md = appendToPCSectionArray(iv->getContext(), bpf_tag_name, old_md, {
    embedU64(iv->getContext(), loop_id)
  });
  iv->setMetadata("pcsections", new_md);

  LLVM_DEBUG(dbgs() << "Tagged iv: ");
  LLVM_DEBUG(iv->print(dbgs()));
  LLVM_DEBUG(dbgs() << "\n");

  this->visit(measure);
}

SCEVCanonicalPrintVisitor::SCEVCanonicalPrintVisitor(ScalarEvolution &SE, Loop *loop, PHINode *iv, uint64_t loop_id) : OS(StringResult), iv(iv), loop(loop), SE(SE), loop_id(loop_id) {}
const std::pair<std::vector<Value *>, std::string>
SCEVCanonicalPrintVisitor::collectResults() {
  return std::pair(this->Result, this->StringResult);
}
void SCEVCanonicalPrintVisitor::printPhi(PHINode *phi) {
  //here we need to obtain all non-phi children of this phi node, so we can register and print them back to back
  //TODO: are these sets actually small and dense?
  llvm::SmallDenseSet<PHINode *> phi_seen;
  llvm::SmallDenseSet<PHINode *> phi_todo;

  phi_todo.insert(phi);

  while (!phi_todo.empty()) {
    auto next = *phi_todo.begin();
    phi_todo.erase(next);
    phi_seen.insert(next);
    for (auto &child : next->incoming_values()) {
      auto val = child.get();
      if (auto found_phi = dyn_cast<PHINode>(val)) {
          if (!phi_seen.contains(found_phi)) {
            phi_todo.insert(found_phi);
          }
      } else {
          printValue(val);
          OS << " ";
      }
    }
  }
}
void SCEVCanonicalPrintVisitor::printValue(Value *val) {
  if (auto phi_node = dyn_cast<PHINode>(val)) {
    //PHI Nodes are more difficult to handle than usual because the instruction they correspond to
    //ceases to exist when compiled, so you can't PCSection to it
    OS << "( phi ";
    printPhi(phi_node);
    OS << ")";
    return;
  }
  //This is where we do our original work
  //TODO: The linter is saying std::find here is bad, but I don't know what to replace it with
  if (std::find(Result.begin(), Result.end(), val) == Result.end()) {
    Result.push_back(val);
  }
  //guaranteed to succeed, we just set the value if it wasn't already there
  // Search method from https://www.geeksforgeeks.org/cpp/std-find-in-cpp/
  //TODO: Maybe a less horrible way to do this?
  uint64_t value_id = std::distance(Result.begin(),std::find(Result.begin(), Result.end(), val));
  OS << "%" << value_id;
}

// This is just a mildly altered copy paste from SCE
void SCEVCanonicalPrintVisitor::visit(const SCEV *S) {
switch (S->getSCEVType()) {
  case scConstant:
    printValue(cast<SCEVConstant>(S)->getValue());
    return;
  case scVScale:
    OS << "vscale";
    return;
  case scPtrToAddr:
  case scPtrToInt: {
    const SCEVCastExpr *PtrCast = cast<SCEVCastExpr>(S);
    const SCEV *Op = PtrCast->getOperand();
    StringRef OpS = S->getSCEVType() == scPtrToAddr ? "addr" : "int";
    OS << "(ptrto" << OpS << " " << *Op->getType() << " " ;
    visit(Op);
    OS << " to " << *PtrCast->getType() << ")";
    return;
  }
  case scTruncate: {
    const SCEVTruncateExpr *Trunc = cast<SCEVTruncateExpr>(S);
    const SCEV *Op = Trunc->getOperand();
    OS << "(trunc " << *Op->getType() << " ";
    visit(Op);
    OS << " to "
       << *Trunc->getType() << ")";
    return;
  }
  case scZeroExtend: {
    const SCEVZeroExtendExpr *ZExt = cast<SCEVZeroExtendExpr>(S);
    const SCEV *Op = ZExt->getOperand();
    OS << "(zext " << *Op->getType() << " ";
    visit(Op);
    OS << " to "
       << *ZExt->getType() << ")";
    return;
  }
  case scSignExtend: {
    const SCEVSignExtendExpr *SExt = cast<SCEVSignExtendExpr>(S);
    const SCEV *Op = SExt->getOperand();
    OS << "(sext " << *Op->getType() << " ";
    visit(Op);
    OS << " to "
       << *SExt->getType() << ")";
    return;
  }
  case scAddRecExpr: {
    //const SCEVAddRecExpr *AR = cast<SCEVAddRecExpr>(S);
    OS << "***ERROR: ADDREC INVOKED - MEASURE CANNOT BE PROPERLY INVARIANT***";
    return;
  }
  case scAddExpr:
  case scMulExpr:
  case scUMaxExpr:
  case scSMaxExpr:
  case scUMinExpr:
  case scSMinExpr:
  case scSequentialUMinExpr: {
    const SCEVNAryExpr *NAry = cast<SCEVNAryExpr>(S);
    const char *OpStr = nullptr;
    switch (NAry->getSCEVType()) {
    case scAddExpr: OpStr = " + "; break;
    case scMulExpr: OpStr = " * "; break;
    case scUMaxExpr: OpStr = " umax "; break;
    case scSMaxExpr: OpStr = " smax "; break;
    case scUMinExpr:
      OpStr = " umin ";
      break;
    case scSMinExpr:
      OpStr = " smin ";
      break;
    case scSequentialUMinExpr:
      OpStr = " umin_seq ";
      break;
    default:
      llvm_unreachable("There are no other nary expression types.");
    }
    OS << "(";
    if (NAry->getNumOperands() > 0) {
      visit(NAry->getOperand(0));
      for (size_t i=1;i<NAry->getNumOperands();i++) {
        OS << OpStr;
        visit(NAry->getOperand(i));
      }
    }
    OS << ")";
    switch (NAry->getSCEVType()) {
    case scAddExpr:
    case scMulExpr:
      if (NAry->hasNoUnsignedWrap())
        OS << "<nuw>";
      if (NAry->hasNoSignedWrap())
        OS << "<nsw>";
      break;
    default:
      // Nothing to print for other nary expressions.
      break;
    }
    return;
  }
  case scUDivExpr: {
    const SCEVUDivExpr *UDiv = cast<SCEVUDivExpr>(S);
    OS << "(";
    visit(UDiv->getLHS());
    OS << " /u ";
    visit(UDiv->getRHS());
    OS << ")";
    return;
  }
  case scUnknown: {
    Value *val = cast<SCEVUnknown>(S)->getValue();
    if (val == this->iv) {
      OS << "%iv";
    } else {
      printValue(val);
    }

    return;
  }
  case scCouldNotCompute:
    OS << "***COULDNOTCOMPUTE***";
    return;
  }
  llvm_unreachable("Unknown SCEV kind!");
}
} // namespace llvm