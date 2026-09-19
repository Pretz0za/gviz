#pragma once

#include "concept/graphLike.hpp"
#include "ecs/exceptions.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/physics/gravity.hpp"
#include "layout/types.hpp"

template <GraphLike G>
GravityForceSystem<G>::GravityForceSystem(G &graph)
    : m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()),
      m_positions(graph.NodeSpace().template GetPool<PositionComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G> void GravityForceSystem<G>::SetMagnitude(double magnitude) {}
template <GraphLike G> void GravityForceSystem<G>::Tick() {}
