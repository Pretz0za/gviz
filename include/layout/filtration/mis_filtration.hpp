#pragma once

#include "concept/graphLike.hpp"
#include "ds/bfs_scratch.hpp"
#include "ds/bitset.hpp"
#include "graph/types.hpp"
#include "layout/filtration/types.hpp"
#include "layout/types.hpp"

template <GraphLike G> class MisFiltrationSystem {
public:
  explicit MisFiltrationSystem(G &graph);
  void Tick();

private:
  void BuildFiltration();
  void BuildFirstLayer(BitSet &vertices);
  bool BuildNextLayer(BitSet &lastLayer);
  void MarkVerticesWithinRadius(NodeID source, uint32_t radius, BitSet &marked);

  BFSScratch *m_scratch;
  G *m_graph;
  DimensionResource m_dimension;
  NestedFiltrationResult *m_output;
};

#include "layout/filtration/mis_filtration.tpp"
