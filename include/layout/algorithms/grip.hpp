#pragma once

#include "concept/graphLike.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/placement/barrycenter.hpp"
#include "layout/types.hpp"

template <GraphLike G> class GRIPLayoutAlgorithm {
public:
  GRIPLayoutAlgorithm(G &graph);

  void RunFiltration();
  void Tick();

private:
  void placeLayer();

  uint32_t m_currLayer;
  DenseNodeSet m_placed;

  DimensionResource m_dimension;
  G *m_graph;
  NestedFiltrationResult *m_filtrationOutput;
  MisFiltrationSystem<G> m_filtrationSystem;
  PositionBarrycentric<G> m_placementSystem;
};

#include "layout/algorithms/grip.tpp"
