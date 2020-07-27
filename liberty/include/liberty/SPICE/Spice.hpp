#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "liberty/Analysis/ControlSpecIterators.h"
#include "liberty/Analysis/ControlSpeculation.h"
#include "liberty/Analysis/LoopAA.h"
#include "liberty/Analysis/SimpleAA.h"
#include "liberty/Orchestration/EdgeCountOracleAA.h"
#include "liberty/Orchestration/PointsToAA.h"
#include "liberty/Orchestration/PredictionSpeculation.h"
#include "liberty/Orchestration/PtrResidueAA.h"
#include "liberty/Orchestration/ReadOnlyAA.h"
#include "liberty/Orchestration/ShortLivedAA.h"
#include "liberty/Orchestration/SmtxAA.h"
#include "liberty/Speculation/CallsiteDepthCombinator_CtrlSpecAware.h"
#include "liberty/Speculation/Classify.h"
#include "liberty/Speculation/KillFlow_CtrlSpecAware.h"
#include "liberty/Speculation/LoopDominators.h"
#include "liberty/Speculation/PredictionSpeculator.h"
#include "liberty/Speculation/Read.h"

#include "PDG.hpp"

#include <unordered_set>
#include <unordered_map>

using namespace llvm;
using namespace liberty;

namespace llvm  {
namespace spice {
struct Spice : public ModulePass {
public:
  static char ID;
  Spice() : ModulePass(ID) {}
  virtual ~Spice() {}

  //void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;


};
}
} // namespace llvm
