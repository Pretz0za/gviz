#pragma once

#include "concept/graphLike.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/physics/gravity.hpp"
#include "layout/placement/randomized.hpp"
#include <cstdint>
#include <memory>

template <GraphLike G, ForceModel F> class ForceDirectedLayoutAlgorithm {

public:
  ForceDirectedLayoutAlgorithm(G &graph);
  void Tick();

private:
  G *m_graph;
  std::unique_ptr<F> m_forceModel;
  std::unique_ptr<GravityForceSystem<G>> m_gravity;
  std::unique_ptr<PositionRandomized<G>> m_randomizer;
  uint8_t m_dimension;
};

#include "layout/algorithms/force_directed.tpp"
