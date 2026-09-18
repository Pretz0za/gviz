#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "layout/helpers/distance.hpp"
#include "layout/types.hpp"

template <GraphLike G>
DistanceCalculationSystem<G>::DistanceCalculationSystem(G &graph)
    : m_positionData(
          (graph.NodeSpace().template GetPool<PositionComponent>())->Data()),
      m_radius(graph.NodeSpace().template GetPool<RadiusComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
  if (!m_radius) {
    m_radius = graph.NodeSpace().template SetPool<RadiusComponent>();
  }
}

template <GraphLike G>
double DistanceCalculationSystem<G>::BetweenNodes(DenseNodeID v,
                                                  DenseNodeID u) {
  // gap between the two node boundaries; can be negative when circles overlap
  double dist = Distance(m_positionData[v.Raw()].pos,
                         m_positionData[u.Raw()].pos, m_dimension);
  return dist - m_radius->Find(v.Raw())->radius -
         m_radius->Find(u.Raw())->radius;
}

template <GraphLike G>
void DistanceCalculationSystem<G>::VecBetweenNodes(DenseNodeID v, DenseNodeID u,
                                                   double *out) {
  double dist = Distance(m_positionData[v.Raw()].pos,
                         m_positionData[u.Raw()].pos, m_dimension);

  Subtract(m_positionData[u.Raw()].pos, m_positionData[v.Raw()].pos, out,
           m_dimension);

  if (IsZero(dist)) {
    ZeroOut(out, m_dimension);
    return;
  }

  double gap =
      dist - m_radius->Find(v.Raw())->radius - m_radius->Find(u.Raw())->radius;
  Scale(out, gap / dist, m_dimension);
}
