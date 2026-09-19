#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/physics/gravity.hpp"
#include "layout/types.hpp"

template <GraphLike G>
GravityForceSystem<G>::GravityForceSystem(G &graph)
    : m_graph(&graph),
      m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()),
      m_positions(graph.NodeSpace().template GetPool<PositionComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G>
void GravityForceSystem<G>::SetMagnitude(double magnitude) {
  m_magnitude = magnitude;
}
template <GraphLike G> void GravityForceSystem<G>::Tick() {
  for (NodeID nid : m_graph->Nodes()) {
    uint32_t degree = m_graph->Degree(nid);
    DenseNodeID denseID = m_graph->MapToDense(nid);

    Vecaxpy(-1 * m_magnitude * static_cast<double>(degree + 1),
            m_positions->Find(denseID.Raw())->pos,
            m_physics->Find(denseID.Raw())->disp, m_dimension);
  }
}
