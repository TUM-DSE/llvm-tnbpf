//
// Created by deniz on 6/23/26.
//

#include "SCEVCanonicalPrintVisitor.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/InterleavedRange.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
namespace llvm {

void SCEVCanonicalPrintVisitor::measure(const SCEV *start, const SCEV *end,
                                        const SCEV *stride) {
  /*
  case Loop::LoopBounds::Direction::Increasing:
  measure_scev = SE.getUDivCeilSCEV(SE.getMinusSCEV(end_scev, iv), stride_scev);
  break;

  case Loop::LoopBounds::Direction::Unknown:
  break;
  */
  auto iv_scev = SE.getSCEV(iv);

  //extremely hacky technique to bypass SCEV's own insistence to transform the PHI node into an AddRec
  iv_being_bootstrapped = true;
  //TODO: maybe instead of doing this: just set iv to %iv and handle separately, still have no clue how to tag this with a register
  printValue(iv);
  OS.flush();
  iv_string = StringResult;
  StringResult = "";




  if (isKnownNegativeInLoop(stride, this->loop, SE)) {

     //calculate iteration number string the hard way
     //TODO: this does not use SCEV itself

    // stride should be invariant, otherwise we have a big problem
    // TODO: haven't got the slightest clue how we deal with polynomials
    // in general: SCEV seems to work with iteration numbers
    // don't know how to reverse engineer the iteration number from the iv in the general case
    // assume iv is in form of start + step * stride with stride invariant
    OS << "((";
    visit(start);
    OS << " - ";
    OS << iv_string;
    OS << ") /u (-";
    visit(stride);
    OS << "))";

    OS.flush();
    iter_no_string = StringResult;
    StringResult = "";

    iv_being_bootstrapped = false;
     this->visit(SE.getUDivCeilSCEV(SE.getMinusSCEV(iv_scev, end), SE.getNegativeSCEV(stride)));

  } else {
    OS << "((";
    OS << iv_string;
    OS << " - ";
    visit(start);
    OS << ") /u ";
    visit(stride);
    OS << ")";

    OS.flush();
    iter_no_string = StringResult;
    StringResult = "";
    iv_being_bootstrapped = false;
    this->visit(SE.getUDivCeilSCEV(SE.getMinusSCEV(end, iv_scev), stride));
  }

}

SCEVCanonicalPrintVisitor::SCEVCanonicalPrintVisitor(ScalarEvolution &SE, Loop *loop, PHINode *iv) : OS(StringResult), iv_string(""), SE(SE), loop(loop), iv(iv), iv_being_bootstrapped(false) {}
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
    const SCEVAddRecExpr *AR = cast<SCEVAddRecExpr>(S);

    // convert AddRec back into regular Add
    if (iv_being_bootstrapped) {
      OS << "***ERROR: ADDREC INVOKED WHILE DERIVING ITERATION NUMBER ***";
      return;
    }

    // flatten the addrec outright
    // lifted from SCEVAddRecExpr::evaluateAtIteration
    /*
    const SCEV *Result = Operands[0].getPointer();
    for (unsigned i = 1, e = Operands.size(); i != e; ++i) {
      // The computation is correct in the face of overflow provided that the
      // multiplication is performed _after_ the evaluation of the binomial
      // coefficient.
      const SCEV *Coeff = BinomialCoefficient(It, i, SE, Result->getType());
      if (isa<SCEVCouldNotCompute>(Coeff))
        return Coeff;

      Result =
          SE.getAddExpr(Result, SE.getMulExpr(Operands[i].getPointer(), Coeff));
    }
    */
    //   BC(It, K) = (It * (It - 1) * ... * (It - K + 1)) / K!




    OS << "(";
    visit(AR->getOperand(0));


    if (AR->getNumOperands() > 0) {
      OS << " + (";
      visit(AR->getOperand(1));
      OS << " * " << iter_no_string << ")";
    }
    unsigned k_fac = 1;
    std::string it_chain = iter_no_string;
    //TODO: no idea if this works for arbitrary polynomials - no idea if it even needs to
    for (unsigned i = 2, e = AR->getNumOperands(); i != e; ++i) {
      k_fac *= i;
      it_chain += " * (" + iter_no_string + " - " + std::to_string(i - 1) + ")";
      OS << " + (";
      visit(AR->getOperand(i));
      OS << " * ((" + it_chain + ") /u " + std::to_string(k_fac) + ")";
    }

    OS << ")";


    if (AR->hasNoUnsignedWrap())
      OS << "<nuw>";
    if (AR->hasNoSignedWrap())
      OS << "<nsw>";
    if (AR->hasNoSelfWrap() && !AR->hasNoUnsignedWrap() &&
        !AR->hasNoSignedWrap())
      OS << "<nw>";
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
      for (int i=1;i<NAry->getNumOperands();i++) {
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
    printValue(cast<SCEVUnknown>(S)->getValue());
    return;
  }
  case scCouldNotCompute:
    OS << "***COULDNOTCOMPUTE***";
    return;
  }
  llvm_unreachable("Unknown SCEV kind!");
}
} // namespace llvm