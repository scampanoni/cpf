#include "llvm/Pass.h"
#include "liberty/Orchestration/SmtxSlampRemed.h"
#include "liberty/Orchestration/SmtxLampRemed.h"
#include "liberty/Analysis/LoopAA.h"
#include "PDG.hpp"

namespace liberty
{
 
void checkConflictsBetweenAnalysisAndProfile(Loop *loop, Pass &proxy,
      SmtxSlampSpeculationManager &smtxMan, SmtxSpeculationManager &smtxLampMan);
 
} // namespace liberty 
