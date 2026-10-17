#pragma once

#include "concept/graphLike.hpp"
#include "ecs/index_space.hpp"
#include "graph/types.hpp"
#include "layout/components/planar_rotation.hpp"

template <GraphLike G> class PlanarRotationSystem {
public:
  explicit PlanarRotationSystem(G &graph);

  void Compute();

private:
  G *m_graph;
  DenseComponentPool<PlanarRotationAdjacencyComponent> *m_rotation;
};

#include "layout/planarity/rotation_system.tpp"
