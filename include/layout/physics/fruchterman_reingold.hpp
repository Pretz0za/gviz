#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/physics.hpp"
#include "layout/helpers/distance.hpp"
#include <cstdint>
class VanillaFruchtermanReingold {};

template <GraphLike G> class GRIPFruchtermanReingold {
public:
  enum MODE {ATTRACTIVE, REPULSIVE};

  GRIPFruchtermanReingold(G &graph);

  void Tick(DenseNodeID v, DenseNodeID u, MODE mode);

private:
  DenseComponentPool<PhysicsComponent> *m_physics;
  DistanceCalculationSystem<G> m_distanceCalc;
  uint8_t m_dimension;
};

#include "layout/physics/fruchterman_reingold.tpp"
