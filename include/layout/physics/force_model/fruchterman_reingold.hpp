#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/helpers/distance.hpp"
#include <cstdint>

template <GraphLike G> class VanillaFruchtermanReingold {
public:
  VanillaFruchtermanReingold(G &graph);

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, const double* uPos, double uMass);
  inline double EdgeLength() const { return m_edgeLength; }

private:
  uint8_t m_dimension;
  double m_edgeLength = 100.0;
  // Floors the distance used for repulsion's magnitude at
  // m_edgeLength * kMinDistFraction, so two vertices landing arbitrarily
  // close together (or exactly coincident) can't spike k^2/dist into an
  // unbounded one-tick impulse. Mirrors grapher-old's VecMinDistFraction.
  static constexpr double kMinDistFraction = 0.01;
  DistanceCalculationSystem<G> m_distanceCalc;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<PhysicsComponent> *m_physics;
};

template <GraphLike G> class GRIPFruchtermanReingold {
public:
  GRIPFruchtermanReingold(G &graph);

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, const double* uPos, double uMass);
  // GRIP's tick formulas hardcode 10.0 as their implicit target spacing.
  inline double EdgeLength() const { return 10.0; }

private:
  DenseComponentPool<PhysicsComponent> *m_physics;
  DistanceCalculationSystem<G> m_distanceCalc;
  uint8_t m_dimension;
};

#include "layout/physics/force_model/fruchterman_reingold.tpp"
static_assert(ForceModel<VanillaFruchtermanReingold<Graph>>);
static_assert(ForceModel<GRIPFruchtermanReingold<Graph>>);
