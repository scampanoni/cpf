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

#include "llvm/ADT/iterator_range.h"

#include "liberty/Analysis/LLVMAAResults.h"
#include "liberty/SPICE/Spice.hpp"
#include "liberty/Strategy/ProfilePerformanceEstimator.h"

#include "Assumptions.h"
#include "Noelle.hpp"

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

bool Spice::runOnModule (Module &M){
  auto& noelle = getAnalysis<Noelle>();
  errs() << "The program has " << noelle.numberOfProgramInstructions() << " instructions\n";
  return false;
}



char Spice::ID = 0;
static RegisterPass< Spice > rp("spice", "spice-transform");
