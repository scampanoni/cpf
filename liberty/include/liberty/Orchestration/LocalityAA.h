// This is an adaptor class from a heap assignment
// to the AA stack.  It applies disjoint heap reasoning
// as a separation AA.  When applied to a PDG, it
// removes edges which will be speculated and validated
// by code that uses that heap assignment.
#ifndef LIBERTY_SPEC_PRIV_LOCALITY_ORACLE_AA_H
#define LIBERTY_SPEC_PRIV_LOCALITY_ORACLE_AA_H

#include "liberty/Analysis/ClassicLoopAA.h"
#include "liberty/Orchestration/Remediator.h"
#include "liberty/Speculation/Classify.h"
#include "liberty/Speculation/Read.h"

namespace liberty
{
using namespace llvm;
using namespace SpecPriv;

/// Adapts separation speculation to LoopAA.
struct LocalityAA : public LoopAA, Remediator // Not a pass!
{
  LocalityAA(const Read &rd, const HeapAssignment &ha, const Ctx *cx,
             PerformanceEstimator *pf)
      : LoopAA(), read(rd), asgn(ha), ctx(cx), perf(pf) {}

  StringRef getLoopAAName() const { return "spec-priv-locality-oracle-aa"; }

  StringRef getRemediatorName() const { return "spec-priv-locality-oracle-remed"; }

  LoopAA::AliasResult alias(const Value *P1, unsigned S1, TemporalRelation rel,
                            const Value *P2, unsigned S2, const Loop *L,
                            Remedies &R,
                            DesiredAliasResult dAliasRes = DNoOrMustAlias);

  LoopAA::ModRefResult modref(const Instruction *A, TemporalRelation rel,
                              const Value *ptrB, unsigned sizeB, const Loop *L,
                              Remedies &R);


  LoopAA::ModRefResult modref(const Instruction *I1, TemporalRelation Rel,
                              const Instruction *I2, const Loop *L,
                              Remedies &remeds);

  LoopAA::ModRefResult modref_with_ptrs(const Instruction *A, const Value *ptrA,
                                        TemporalRelation rel,
                                        const Instruction *B, const Value *ptrB,
                                        const Loop *L, Remedies &R);

  LoopAA::SchedulingPreference getSchedulingPreference() const {
    return SchedulingPreference(Bottom + 4);
  }

private:
  const Read &read;
  const HeapAssignment &asgn;
  const Ctx *ctx;
  PerformanceEstimator *perf;

  unordered_set<const Value*> privateInsts;

  void populateCheapPrivRemedies(Ptrs aus, Remedies &R);
  void populateNoWAWRemedies(Ptrs aus, Remedies &R);
};

}

#endif

