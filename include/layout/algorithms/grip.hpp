#pragma once

#include "concept/graphLike.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/types.hpp"

template <GraphLike G> class GRIPLayoutAlgorithm {
public:
  GRIPLayoutAlgorithm(G &graph);

  void RunFiltration();
  void Tick();

private:
  DimensionResource m_dimension;
  G *m_graph;
  NestedFiltrationResult *m_filtrationOutput;
  MisFiltrationSystem<G> m_filtrationSystem;
  BitSet m_placed;
};

#include "layout/algorithms/grip.tpp"
