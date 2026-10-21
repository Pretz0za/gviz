#pragma once

#include "concept/graphLike.hpp"
#include "graph/types.hpp"
#include "layout/planarity/rotation_system.hpp"
#include <array>
#include <cstddef>
#include <vector>

template <GraphLike G> class SchnyderWood {
public:
  SchnyderWood(PlanarRotationSystem<G> &rotation, NodeID s0, NodeID s1);

  NodeID Root(size_t tree) const noexcept { return m_root[tree]; }
  NodeID Parent(size_t tree, NodeID v) const;

private:
  G &m_g;
  std::array<NodeID, 3> m_root;
  std::array<std::vector<NodeID>, 3> m_parent;
};

#include "layout/planarity/schnyder_wood.tpp"
