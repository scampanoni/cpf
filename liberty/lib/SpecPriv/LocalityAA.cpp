#define DEBUG_TYPE "spec-priv-locality-aa"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/Statistic.h"

#include "LocalityAA.h"

namespace liberty
{
namespace SpecPriv
{
using namespace llvm;

STATISTIC(numQueries,    "Num queries");
STATISTIC(numEligible,   "Num eligible queries");
STATISTIC(numPrivatized, "Num privatized");
STATISTIC(numSeparated,  "Num separated");
STATISTIC(numSubSep,     "Num separated via subheaps");
STATISTIC(numSubheapOpportunities, "Num sub-heap opportunities");

cl::opt<bool> CheckForSubheapOpportunities(
  "check-for-subheap-opportunities", cl::init(false), cl::Hidden,
  cl::desc("Check for sub-heap separation opportunities"));


LoopAA::AliasResult LocalityAA::aliasCheck(
    const Pointer &P1,
    TemporalRelation rel,
    const Pointer &P2,
    const Loop *L)
{
  ++numQueries;

  if( !L || !asgn.isValidFor(L) )
    return MayAlias;

  if( !isa<PointerType>( P1.ptr->getType() ) )
    return MayAlias;
  if( !isa<PointerType>( P2.ptr->getType() ) )
    return MayAlias;

  const Ctx *ctx = read.getCtx(L);

  ++numEligible;

  Ptrs aus1;
  HeapAssignment::Type t1 = HeapAssignment::Unclassified;
  if( read.getUnderlyingAUs(P1.ptr,ctx,aus1) )
    t1 = asgn.classify(aus1);

  Ptrs aus2;
  HeapAssignment::Type t2 = HeapAssignment::Unclassified;
  if( read.getUnderlyingAUs(P2.ptr,ctx,aus2) )
    t2 = asgn.classify(aus2);

  // Loop-carried queries:
  if( rel != LoopAA::Same )
  {
    // Reduction, local and private heaps are iteration-private, thus
    // there cannot be cross-iteration flows.
    if( t1 == HeapAssignment::Redux || t1 == HeapAssignment::Local || t1 == HeapAssignment::Private )
    {
      ++numPrivatized;
      return NoAlias;
    }

    if( t2 == HeapAssignment::Redux || t2 == HeapAssignment::Local || t2 == HeapAssignment::Private )
    {
      ++numPrivatized;
      return NoAlias;
    }
  }

  // Both loop-carried and intra-iteration queries: are they assigned to different heaps?
  if( t1 != t2 && t1 != HeapAssignment::Unclassified && t2 != HeapAssignment::Unclassified )
  {
    ++numSeparated;
    return NoAlias;
  }

  // They are assigned to the same heap.
  // Are they assigned to different sub-heaps?
  if( t1 == t2 && t1 != HeapAssignment::Unclassified )
  {
    //sdflsdakjfjsdlkjfl
    const int subheap1 = asgn.getSubHeap(aus1);
    if( subheap1 > 0 )
    {
      const int subheap2 = asgn.getSubHeap(aus2);
      if( subheap2 > 0 && subheap1 != subheap2 )
      {
        ++numSubSep;
        return NoAlias;
      }
    }
  }

  // At this point, separation speculation cannot disambiguate
  // these pointers because they are assignend to the same heap.

  // In the future, I might implement sub-heap separation.
  // Before I do that, I want to estimate the benefit.
  if( CheckForSubheapOpportunities )
  {
    // We'll check if the points-to sets are disjoint.
    // This is a necessary condition for sub-heap separation to
    // help.
    bool disjoint = true;
    for(unsigned i=0, N=aus1.size(); i<N && disjoint; ++i)
      for(unsigned j=0, M=aus2.size(); j<M && disjoint; ++j)
        if( (*aus1[i].au) == (*aus2[j].au) )
          disjoint = false;

    if( disjoint )
    {
      // Also, it's only beneficial if the no-alias relationship
      // cannot be decided by static analysis alone.
      if( LoopAA::alias(P1.ptr,P1.size,rel,P2.ptr,P2.size,L) != LoopAA::NoAlias )
      {
        DEBUG(errs() << "Pointers cannot be disambiguated with separation speculation,\n"
                     << "yet may benefit from sub-heap separation\n"
                     << " - " << *P1.ptr << '\n'
                     << " - " << *P2.ptr << '\n');
        ++numSubheapOpportunities;
      }
    }
  }

  return MayAlias;
}


}
}

