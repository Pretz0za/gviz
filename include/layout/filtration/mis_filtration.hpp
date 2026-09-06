#pragma once

#include "concept/graphLike.hpp"
#include "ds/bfs_scratch.hpp"
#include "ds/bitset.hpp"
#include "layout/filtration/types.hpp"
#include "layout/types.hpp"

template <GraphLike G> class MisFiltrationSystem {
public:
  explicit MisFiltrationSystem(G &graph);
  void Tick(NestedFiltrationResult &out);

private:
  void BuildFiltration(NestedFiltrationResult &out);
  void BuildFirstLayer(NestedFiltrationResult &out, BitSet &vertices);
  bool BuildNextLayer(NestedFiltrationResult &out, BitSet &lastLayer);
  void MarkVerticesWithinRadius(uint32_t source, uint32_t radius, BitSet &marked);

  BFSScratch *m_scratch;
  G *m_graph;
  DimensionResource m_dimension;
};

#include "layout/filtration/mis_filtration.tpp"
