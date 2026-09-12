#pragma once

#include "concept/graphLike.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/placement/barrycenter.hpp"
#include "layout/types.hpp"
#include <memory>

// implementation of the GRIP algorithm by Gajer and Kobourov
// https://www2.cs.arizona.edu/~kobourov/grip_demo.pdf.
template <GraphLike G> class GRIPLayoutAlgorithm {
public:
  GRIPLayoutAlgorithm(G &graph);

  void RunFiltration();
  uint32_t TransitionState();
  void Tick();

private:
  void placeLayer();

  uint32_t m_currLayer;
  DenseNodeSet *m_visible;
  uint8_t m_dimension;
  G *m_graph;
  NestedFiltrationResult *m_filtrationOutput;
  std::unique_ptr<MisFiltrationSystem<G>> m_filtrationSystem = nullptr;
  std::unique_ptr<PositionBarrycentric<G>> m_placementSystem = nullptr;
};

#include "layout/algorithms/grip.tpp"
