#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"

template <GraphLike G> class DistanceCalculationSystem {
public:
  DistanceCalculationSystem(G &graph);

  inline double BetweenNodes(DenseNodeID v, DenseNodeID u);

  // u - v -> out
  inline void VecBetweenNodes(DenseNodeID v, DenseNodeID u, double *out);

private:
  DenseComponentPool<RadiusComponent> *m_radius;
  PositionSpan &m_positionData;
  uint8_t m_dimension;
};

#include "layout/helpers/distance.tpp"
