#define DEBUG_TYPE "selector"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "liberty/Analysis/EdgeCountOracleAA.h"
#include "liberty/LoopProf/Targets.h"
#include "liberty/SpecPriv/ControlSpeculator.h"
#include "liberty/SpecPriv/Pipeline.h"
#include "liberty/SpecPriv/PredictionSpeculator.h"
#include "liberty/SpecPriv/ProfilePerformanceEstimator.h"
#include "liberty/SpecPriv/Read.h"
#include "liberty/Utilities/CallSiteFactory.h"
#include "liberty/Utilities/ModuleLoops.h"

#include "PtrResidueAA.h"
#include "LocalityAA.h"
#include "RoI.h"
#include "PrivateerSelector.h"
#include "UpdateOnCloneAdaptors.h"
//#include "Transform.h"


namespace liberty
{
namespace SpecPriv
{

const HeapAssignment &SpecPrivSelector::getAssignment() const { return assignment; }
HeapAssignment &SpecPrivSelector::getAssignment() { return assignment; }

void SpecPrivSelector::computeVertices(Vertices &vertices)
{
  ModuleLoops &mloops = getAnalysis< ModuleLoops >();
  const Classify &classify = getAnalysis< Classify >();
  for(Classify::iterator i=classify.begin(), e=classify.end(); i!=e; ++i)
  {
    const BasicBlock *header = i->first;
    Function *fcn = const_cast< Function * >(header->getParent() );

    LoopInfo &li = mloops.getAnalysis_LoopInfo(fcn);
    Loop *loop = li.getLoopFor(header);
    assert( loop->getHeader() == header );

    const HeapAssignment &asgn = i->second;
    if( ! asgn.isValidFor(loop) )
      continue;

    vertices.push_back( loop );
  }
}

void SpecPrivSelector::getAnalysisUsage(AnalysisUsage &au) const
{
  Selector::analysisUsage(au);

  au.addRequired< ReadPass >();
  au.addRequired< ProfileGuidedControlSpeculator >();
  au.addRequired< ProfileGuidedPredictionSpeculator >();
  au.addRequired< PtrResidueSpeculationManager >();
  au.addRequired< Classify >();
}


bool SpecPrivSelector::compatibleParallelizations(const Loop *A, const Loop *B) const
{
  const Classify &classify = getAnalysis< Classify >();

  const HeapAssignment &asgnA = classify.getAssignmentFor(A);
  assert( asgnA.isValidFor(A) );

  const HeapAssignment &asgnB = classify.getAssignmentFor(B);
  assert( asgnB.isValidFor(B) );

  return compatible(asgnA, asgnB);
}

ControlSpeculation *SpecPrivSelector::getControlSpeculation() const
{
  return getAnalysis< ProfileGuidedControlSpeculator >().getControlSpecPtr();
}

PredictionSpeculation *SpecPrivSelector::getPredictionSpeculation() const
{
  return &getAnalysis< ProfileGuidedPredictionSpeculator >();
}

void SpecPrivSelector::buildSpeculativeAnalysisStack(const Loop *A)
{
  //DataLayout &td = getAnalysis< DataLayout >();
  Module* mod = A->getHeader()->getParent()->getParent();
  const DataLayout &td = mod->getDataLayout();
  PtrResidueSpeculationManager &prman = getAnalysis< PtrResidueSpeculationManager >();
  ControlSpeculation *ctrlspec = getControlSpeculation();
  PredictionSpeculation *predspec = getPredictionSpeculation();
  const Read &spresults = getAnalysis< ReadPass >().getProfileInfo();
  Classify &classify = getAnalysis< Classify >();

  const HeapAssignment &asgnA = classify.getAssignmentFor(A);
  assert( asgnA.isValidFor(A) );

  // Pointer-residue speculation
  residueaa = new PtrResidueAA(td,prman);
  residueaa->InitializeLoopAA(this, td);

  // Value predictions
  predaa = new PredictionAA(predspec);
  predaa->InitializeLoopAA(this, td);

  // Control Speculation
  ctrlspec->setLoopOfInterest(A->getHeader());
  edgeaa = new EdgeCountOracle(ctrlspec);
  edgeaa->InitializeLoopAA(this, td);

  // Apply locality reasoning (i.e. an object is local/private to a context,
  // and thus cannot source/sink loop-carried deps).
  localityaa = new LocalityAA(spresults,asgnA);
  localityaa->InitializeLoopAA(this, td);
}

void SpecPrivSelector::destroySpeculativeAnalysisStack()
{
  delete localityaa;
  delete edgeaa;
  delete predaa;
  delete residueaa;

  ControlSpeculation *ctrlspec = getControlSpeculation();
  ctrlspec->setLoopOfInterest(0);
}

bool SpecPrivSelector::runOnModule(Module &mod)
{
  DEBUG_WITH_TYPE("classify",
    errs() << "#################################################\n"
           << " Selection\n\n\n");

  Vertices vertices;
  Edges edges;
  VertexWeights weights;
  VertexSet maxClique;

  doSelection(vertices, edges, weights, maxClique);

  // Combine all of these assignments into one big assignment
  Classify &classify = getAnalysis< Classify >();
  for(VertexSet::iterator i=maxClique.begin(), e=maxClique.end(); i!=e; ++i)
  {
    const unsigned v = *i;
    Loop *l = vertices[ v ];
    const HeapAssignment &asgn = classify.getAssignmentFor(l);
    assert( asgn.isValidFor(l) );

    assignment = assignment & asgn;
  }

  DEBUG_WITH_TYPE("classify", errs() << assignment );
  return false;
}

void SpecPrivSelector::resetAfterInline(
  Instruction *callsite_no_longer_exists,
  Function *caller,
  Function *callee,
  const ValueToValueMapTy &vmap,
  const CallsPromotedToInvoke &call2invoke)
{
  Classify &classify = getAnalysis< Classify >();
  PtrResidueSpeculationManager &prman = getAnalysis< PtrResidueSpeculationManager >();
  ControlSpeculation *ctrlspec = getAnalysis< ProfileGuidedControlSpeculator >().getControlSpecPtr();
  ProfileGuidedPredictionSpeculator &predspec = getAnalysis< ProfileGuidedPredictionSpeculator >();
  LAMPLoadProfile &lampprof = getAnalysis< LAMPLoadProfile >();
  Read &spresults = getAnalysis< ReadPass >().getProfileInfo();

  UpdateLAMP lamp( lampprof );

  UpdateGroup group;
  group.add( &spresults );
  group.add( &classify );


  FoldManager &fmgr = * spresults.getFoldManager();

  // Hard to identify exactly which context we're updating,
  // since the context includes loops and functions, but not callsites.

  // Find every context in which 'callee' is called by 'caller'
  typedef std::vector<const Ctx *> Ctxs;
  Ctxs affectedContexts;
  for(FoldManager::ctx_iterator k=fmgr.ctx_begin(), z=fmgr.ctx_end(); k!=z; ++k)
  {
    const Ctx *ctx = &*k;
    if( ctx->type != Ctx_Fcn )
      continue;
    if( ctx->fcn != callee )
      continue;

    if( !ctx->parent )
      continue;
    if( ctx->parent->getFcn() != caller )
      continue;

    affectedContexts.push_back( ctx );
    errs() << "Affected context: " << *ctx << '\n';
  }

  // Inline those contexts to build the cmap, amap
  CtxToCtxMap cmap;
  AuToAuMap amap;
  for(Ctxs::const_iterator k=affectedContexts.begin(), z=affectedContexts.end(); k!=z; ++k)
    fmgr.inlineContext(*k,vmap,cmap,amap);

  lamp.resetAfterInline(callsite_no_longer_exists, caller, callee, vmap, call2invoke);

  // Update others w.r.t each of those contexts
  for(Ctxs::const_iterator k=affectedContexts.begin(), z=affectedContexts.end(); k!=z; ++k)
    group.contextRenamedViaClone(*k,vmap,cmap,amap);

  spresults.removeInstruction( callsite_no_longer_exists );

  predspec.reset();
  prman.reset();
  ctrlspec->reset();
}

void SpecPrivSelector::contextRenamedViaClone(
  const Ctx *changedContext,
  const ValueToValueMapTy &vmap,
  const CtxToCtxMap &cmap,
  const AuToAuMap &amap)
{
//  errs() << "  . . - Selector::contextRenamedViaClone: " << *changedContext << '\n';
  assignment.contextRenamedViaClone(changedContext,vmap,cmap,amap);
  Selector::contextRenamedViaClone(changedContext,vmap,cmap,amap);
}


char SpecPrivSelector::ID = 0;
static RegisterPass< SpecPrivSelector > default_instance("spec-priv-selector", "SpecPriv Selector");
static RegisterAnalysisGroup< Selector > link(default_instance);

}
}
