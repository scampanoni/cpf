#ifndef LLVM_LIBERTY_COUNTED_IV_REMED_H
#define LLVM_LIBERTY_COUNTED_IV_REMED_H

#include "llvm/IR/Instructions.h"

#include "liberty/Utilities/ModuleLoops.h"
#include "liberty/Analysis/LoopAA.h"
#include "liberty/Orchestration/Remediator.h"
#include "PDG.hpp"
#include "Noelle.hpp"

namespace liberty {
using namespace llvm;

class CountedIVRemedy : public Remedy {
public:
  const PHINode *ivPHI;

  void apply(Task *task);
  bool compare(const Remedy_ptr rhs) const;
  StringRef getRemedyName() const { return "counted-iv-remedy"; };
};

class CountedIVRemediator : public Remediator {
public:
  CountedIVRemediator(LoopDependenceInfo* LDI) : Remediator(), ldi(LDI) {}

  StringRef getRemediatorName() const { return "counted-iv-remediator"; }

  RemedResp regdep(const Instruction *A, const Instruction *B, bool loopCarried);

  RemedResp ctrldep(const Instruction *A, const Instruction *B);

private:
  LoopDependenceInfo *ldi;
};

} // namespace liberty

#endif
