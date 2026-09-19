#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include "layout/components/physics.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/helpers/distance.hpp"
#include <cstdint>

template <GraphLike G> class VanillaFruchtermanReingold {
public:
  VanillaFruchtermanReingold(G &graph);

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);

private:
};

template <GraphLike G> class GRIPFruchtermanReingold {
public:
  GRIPFruchtermanReingold(G &graph);

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);

private:
  DenseComponentPool<PhysicsComponent> *m_physics;
  DistanceCalculationSystem<G> m_distanceCalc;
  uint8_t m_dimension;
};

#include "layout/physics/fruchterman_reingold.tpp"
static_assert(ForceModel<VanillaFruchtermanReingold<Graph>>);
static_assert(ForceModel<GRIPFruchtermanReingold<Graph>>);
