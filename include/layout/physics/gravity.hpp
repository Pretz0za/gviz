#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"

template <GraphLike G> class GravityForceSystem {

public:
  GravityForceSystem(G &graph);

  inline void Tick();
  void SetMagnitude(double magnitude);

private:
  G *m_graph;
  DenseComponentPool<PhysicsComponent> *m_physics;
  DenseComponentPool<PositionComponent> *m_positions;
  uint8_t m_dimension;
  double m_magnitude = 0.2;
};
#include "layout/physics/gravity.tpp"

