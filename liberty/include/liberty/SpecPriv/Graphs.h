#ifndef LLVM_LIBERTY_SPEC_PRIV_GRAPHS_H
#define LLVM_LIBERTY_SPEC_PRIV_GRAPHS_H

#include <vector>
#include <set>
#include <map>

#include "Graphs.h"

namespace liberty
{
namespace SpecPriv
{
  typedef unsigned Vertex;
  typedef std::vector<Vertex> VertexSet;
  typedef std::pair<Vertex,Vertex> Edge;
  typedef std::set<Edge> Edges;
  typedef int VertexWeight;
  typedef std::vector<VertexWeight> VertexWeights;
  typedef unsigned EdgeWeight;
  typedef std::map<Edge,EdgeWeight> EdgeWeights;

  typedef std::map<Vertex,VertexSet> Adjacencies;

  extern const EdgeWeight Infinity;
  extern const Vertex Source;
  extern const Vertex Sink;
}
}

#endif
