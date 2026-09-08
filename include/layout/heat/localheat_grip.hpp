#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/physics.hpp"

// implementation of the local temperature described in GRIP
// https://www2.cs.arizona.edu/~kobourov/grip_demo.pdf page 5.
template <GraphLike G> class LocalHeatGrip {
public:
  LocalHeatGrip(G &graph);

  void Tick(DenseNodeID id);

private:
  DenseComponentPool<PhysicsComponent> *m_physicsPool;
  DenseComponentPool<LocalHeatComponent> *m_heatPool;
  uint8_t m_dimension;
};

#include "layout/heat/localheat_grip.tpp"
