#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "layout/components/physics.hpp"

template <GraphLike G> class GravityForceSystem {

public:
  GravityForceSystem(G &graph);

  inline void Tick();
  void SetMagnitude(double magnitude);

private:
  DenseComponentPool<PhysicsComponent> *m_physics;
  DenseComponentPool<PhysicsComponent> *m_positions;
  uint8_t m_dimension;
};
#include "layout/physics/gravity.tpp"

