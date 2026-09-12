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
    : m_positionPool(graph.NodeSpace().template GetPool<PositionComponent>()) {
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
  auto &data = m_positionPool->Data();
  return Distance(data[v.Raw()].pos, data[u.Raw()].pos, m_dimension);
}

template <GraphLike G>
void DistanceCalculationSystem<G>::VecBetweenNodes(DenseNodeID v,
                                                  DenseNodeID u, double *out) {
  auto &data = m_positionPool->Data();
  Subtract(data[u.Raw()].pos, data[v.Raw()].pos, out, m_dimension);
}
