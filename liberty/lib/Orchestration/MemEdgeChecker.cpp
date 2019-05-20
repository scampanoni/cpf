#define DEBUG_TYPE   "memEdgeChecker"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Analysis/TargetLibraryInfo.h"

#include "liberty/Utilities/CallSiteFactory.h"
#include "liberty/Utilities/GetMemOper.h"
#include "liberty/Analysis/LoopAA.h"
#include "liberty/Orchestration/Remediator.h"
#include "liberty/Orchestration/SmtxSlampRemed.h"
#include "liberty/Orchestration/SmtxLampRemed.h"
#include "liberty/SLAMP/SLAMPLoad.h"
#include "PDG.hpp"

#include "liberty/Orchestration/MemEdgeChecker.h"

namespace liberty
{
  using namespace llvm;
  
  static bool isMemIntrinsic(const Instruction *inst) {
    return isa<MemIntrinsic>(inst);
  }

  static bool intrinsicMayRead(const Instruction *inst) {
    ImmutableCallSite cs(inst);
    StringRef name = cs.getCalledFunction()->getName();
    if (name == "llvm.memset.p0i8.i32" || name == "llvm.memset.p0i8.i64")
      return false;

    return true;
  }

  // Since SLAMP and LAMP only care about RaW deps
  static bool noMemoryDepWithAnalysis(const Instruction *src, const Instruction *dst,
                               LoopAA::TemporalRelation FW,
                               LoopAA::TemporalRelation RV, const Loop *loop,
                               LoopAA *aa, bool rawDep) {
    // forward dep test
    LoopAA::ModRefResult forward = aa->modref(src, FW, dst, loop);
    if (LoopAA::NoModRef == forward){
      DEBUG(errs()  << "NoModRef Forward:\n" << *src << "\n->" << *dst << "\n");
      return true;
    }

    // forward is Mod, ModRef, or Ref

    // reverse dep test
    LoopAA::ModRefResult reverse = forward;

    if (src != dst)
      reverse = aa->modref(dst, RV, src, loop);

    if (LoopAA::NoModRef == reverse){
      DEBUG(errs() << "NoModRef Reverse\n"<< *src << "\n->" << *dst << "\n");
      return true;
    }

    if (LoopAA::Ref == forward && LoopAA::Ref == reverse){
      DEBUG(errs() << "RAR\n"<< *src << "\n->" << *dst << "\n");
      return true; // RaR dep; who cares.
    }

    // At this point, we know there is one or more of
    // a flow-, anti-, or output-dependence.

    bool RAW = (forward == LoopAA::Mod || forward == LoopAA::ModRef) &&
               (reverse == LoopAA::Ref || reverse == LoopAA::ModRef);
    bool WAR = (forward == LoopAA::Ref || forward == LoopAA::ModRef) &&
               (reverse == LoopAA::Mod || reverse == LoopAA::ModRef);
    bool WAW = (forward == LoopAA::Mod || forward == LoopAA::ModRef) &&
               (reverse == LoopAA::Mod || reverse == LoopAA::ModRef);

    if (rawDep && !RAW){
      DEBUG(errs() << "Not a RAW\n"<< *src << "\n->" << *dst << "\n");
      return true;
    }

    if (!rawDep && !WAR && !WAW)
      return true;

    return false;
  }

  // Print conflict edges
  static void printConflicts(Instruction *outgoingI, Instruction *incomingI){
    errs() << *outgoingI;
    liberty::printInstDebugInfo(outgoingI);
    errs() << " ->\n";
    errs() << *incomingI;
    liberty::printInstDebugInfo(incomingI);
    errs() << "\n";
  }
  // probably a good idea to reuse the criticisms interface
  // to identify potential conflicts
  void checkConflictsBetweenAnalysisAndProfile(Loop *loop, Pass &proxy,
      SmtxSlampSpeculationManager &smtxMan, SmtxSpeculationManager &smtxLampMan){

    BasicBlock *header = loop->getHeader();
    Function *fcn = header->getParent();

    errs() << "!!!!!!!!!\nCONFLICTS for " << fcn->getName() << "::" << header->getName() << ":\n";
    int conflictCnt = 0;
    // if not pdg 
    auto pdg = std::make_unique<llvm::PDG>();
    pdg->populateNodesOf(loop);

    // Not sure about this part
    // Loading LoopAA

    LoopAA *loopAA = proxy.getAnalysis<LoopAA>().getTopAA();
    // loopAA->dump();

    // Get Lamp and Slamp
    // memory specualation remediator 1 (with SLAMP)
    auto slampR = std::make_unique<SmtxSlampRemediator>(&smtxMan);
    slamp::SLAMPLoadProfile &slamp = smtxMan.getSlampResult();

    // memory specualation remediator 2 (with LAMP)
    auto lampR = std::make_unique<SmtxLampRemediator>(&smtxLampMan, proxy);

    for (auto nodeI : make_range(pdg->begin_nodes(), pdg->end_nodes())) {
      Value *pdgValueI = nodeI->getT();
      Instruction *sop = dyn_cast<Instruction>(pdgValueI);
      assert(sop && "Expecting an instruction as the value of a PDG node");

      if (!sop->mayReadOrWriteMemory())
        continue;

      if (!(isa<StoreInst>(sop) || isMemIntrinsic(sop) || isa<CallInst>(sop))) {
          // Inapplicable
          continue;
        }


      for (auto nodeJ : make_range(pdg->begin_nodes(), pdg->end_nodes())) {
        Value *pdgValueJ = nodeJ->getT();
        Instruction *dop = dyn_cast<Instruction>(pdgValueJ);
        assert(dop && "Expecting an instruction as the value of a PDG node");

        if (!dop->mayReadOrWriteMemory())
          continue;

        // Slamp profile data is colected for loads, stores, and callistes.
        // Slamp only collect FLOW info.
        // Thus, we are looking for Store/CallSite -> Load/CallSite
        // TODO: what about lamp?

        if (!(isa<LoadInst>(dop) || (isMemIntrinsic(dop) && intrinsicMayRead(dop)) ||
              isa<CallInst>(dop))) {
          // Inapplicable
          continue;
        }

        bool raw = true;
        bool lc = false;
        if (noMemoryDepWithAnalysis(sop, dop, LoopAA::Same, LoopAA::Same, loop, loopAA, true)){
          // test slamp memdep

          if (slamp.isTargetLoop(loop)) {
            Remediator::RemedResp slampRemedResp = slampR->memdep(sop, dop, lc, raw, loop);
            if (slampRemedResp.depRes != DepResult::NoDep){
              errs() << "SLAMP Disagrees II Dep: ";
              printConflicts(sop, dop);
              conflictCnt++;
            }
          }

          // test lamp memdep
          // TODO: For LAMP, have to figure out a way to turn off CAF (don't chain)
          Remediator::RemedResp lampRemedResp = lampR->memdep(sop, dop, lc, raw, loop);
          if (lampRemedResp.depRes != DepResult::NoDep){
            errs() << "LAMP Disagrees II Dep: ";
            printConflicts(sop, dop);
            conflictCnt++;
          }
        }

        lc = true;
        if (noMemoryDepWithAnalysis(sop, dop, LoopAA::Before, LoopAA::After, loop, loopAA, true)){
          // test slamp memdep

          if (slamp.isTargetLoop(loop)) {
            Remediator::RemedResp slampRemedResp = slampR->memdep(sop, dop, lc, raw, loop);
            if (slampRemedResp.depRes != DepResult::NoDep){
              errs() << "SLAMP Disagrees LC Dep: ";
              printConflicts(sop, dop);
              conflictCnt++;
            }
          }
              
          // test lamp memdep
          // TODO: For LAMP, have to figure out a way to turn off CAF (don't chain)
          Remediator::RemedResp lampRemedResp = lampR->memdep(sop, dop, lc, raw, loop);
          if (lampRemedResp.depRes != DepResult::NoDep){
            errs() << "LAMP Disagrees LC Dep: ";
            printConflicts(sop, dop);
            conflictCnt++;
          }
        }
 
      }
    }
 
    errs() << "Conflict Count for " << fcn->getName() << "::" << header->getName()
                              << " : " << conflictCnt << "\n!!!!!!!!!\n";
  }
 
} // namespace liberty
