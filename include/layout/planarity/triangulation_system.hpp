#pragma once

#include "concept/graphLike.hpp"
#include "layout/planarity/rotation_system.hpp"

template <GraphLike G> class TriangulationSystem {
public:
  explicit TriangulationSystem(PlanarRotationSystem<G> &rotation);

  void Compute();

private:
  PlanarRotationSystem<G> *m_rotation;
};

#include "layout/planarity/triangulation_system.tpp"
