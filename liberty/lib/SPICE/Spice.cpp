/*TODOs:
 * Identify loop using metadata, match that with a loop in noelle
 */
#define DEBUG_TYPE "pdgbuilder"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "liberty/SPICE/Spice.hpp"
#include "liberty/LoopProf/Targets.h"
#include "liberty/Utilities/ModuleLoops.h"

using namespace llvm;
using namespace liberty;
using namespace spice;
/*void llvm::PDGBuilder::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired< LoopAA >();
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.addRequired<LLVMAAResults>();
  AU.addRequired<ProfileGuidedControlSpeculator>();
  AU.addRequired<ProfileGuidedPredictionSpeculator>();
  AU.addRequired<SmtxSpeculationManager>();
  AU.addRequired<PtrResidueSpeculationManager>();
  AU.addRequired<ReadPass>();
  AU.addRequired<Classify>();
  AU.addRequired<KillFlow_CtrlSpecAware>();
  AU.addRequired<CallsiteDepthCombinator_CtrlSpecAware>();
  AU.addRequired< ProfilePerformanceEstimator >();
  AU.setPreservesAll();
}*/

/*static cl::opt<std::string> ExplicitTargetFcn(
  "target-fcn", cl::init(""), cl::NotHidden,
  cl::desc("Explicit Target Function"));*/

bool Spice::runOnModule (Module &M){
  ModuleLoops &mloops = getAnalysis< ModuleLoops >();
  //const Targets &targets = getAnalysis< Targets >();
  //for(Targets::iterator i=targets.begin(mloops), e=targets.end(mloops); i!=e; ++i)
  //{
  //  Loop *loop = *i;
  //  loop->dump();
  //}
 /* for (Module::iterator func = M.begin(), func_end = M.end(); func != func_end; ++func)
      for (Function::iterator bb = func->begin(), bb_end = func->end(); bb != bb_end; ++bb)
          for (BasicBlock::iterator inst = bb->begin(), inst_end = bb->end(); inst != inst_end; inst++)
              if(inst->getMetadata("note.noelle"))
                errs() << *inst << "\n";*/
  return false;
}



char Spice::ID = 0;
static RegisterPass< Spice > rp("spice", "spice-transform");