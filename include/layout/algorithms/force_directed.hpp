#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "layout/components/physics.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/placement/randomized.hpp"
#include <memory>

template <GraphLike G, ForceModel F> class ForceDirectedLayoutAlgorithm {

public:
  ForceDirectedLayoutAlgorithm(G &graph, F &forceModel);
  void Tick();

private:
  G *m_graph;
  std::unique_ptr<F> m_forceModel;
  std::unique_ptr<GravityForceSystem> m_gravity;
  PositionRandomized<G> m_randomizer;
  DenseComponentPool<PhysicsComponent> m_physics;
};
