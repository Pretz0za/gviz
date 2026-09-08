#pragma once

#include "concept/graphLike.hpp"
#include "ds/bfs_scratch.hpp"
#include "graph/types.hpp"
#include "layout/filtration/types.hpp"
#include "layout/types.hpp"

template <GraphLike G> class MisFiltrationSystem {
public:
  explicit MisFiltrationSystem(G &graph);
  void Tick();

private:
  void BuildFiltration();
  void BuildFirstLayer(DenseNodeSet &vertices);
  bool BuildNextLayer(DenseNodeSet &lastLayer);
  void MarkVerticesWithinRadius(NodeID source, uint32_t radius,
                                DenseNodeSet &marked);

  BFSScratch *m_scratch;
  G *m_graph;
  uint8_t m_dimension;
  NestedFiltrationResult *m_output;
};

#include "layout/filtration/mis_filtration.tpp"
