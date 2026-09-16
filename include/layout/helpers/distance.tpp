#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/helpers/distance.hpp"
#include "layout/types.hpp"

template <GraphLike G>
DistanceCalculationSystem<G>::DistanceCalculationSystem(G &graph)
    : m_positionData((graph.NodeSpace().template GetPool<PositionComponent>())->Data()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G>
double DistanceCalculationSystem<G>::BetweenNodes(DenseNodeID v,
                                                  DenseNodeID u) {
  // NOTE: if vertex radius will be taken into account here, change how vector
  // difference is calculated as well. 
  // pos[v] - pos[u] --> gap/dist * (pos[v] - pos[u])
  return Distance(m_positionData[v.Raw()].pos, m_positionData[u.Raw()].pos, m_dimension);
}

template <GraphLike G>
void DistanceCalculationSystem<G>::VecBetweenNodes(DenseNodeID v,
                                                  DenseNodeID u, double *out) {
  Subtract(m_positionData[u.Raw()].pos, m_positionData[v.Raw()].pos, out, m_dimension);
}
