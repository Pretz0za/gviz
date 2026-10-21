#pragma once

#include "concept/graphLike.hpp"
#include "ecs/index_space.hpp"
#include "graph/types.hpp"
#include "layout/components/planar_rotation.hpp"
#include <cstddef>
#include <vector>

template <GraphLike G> class PlanarRotationSystem {
public:
  explicit PlanarRotationSystem(G &graph);

  void Compute();

  G &Graph() const { return *m_graph; }

  const std::vector<AdjEntry> &Neighbors(NodeID v) const;
  size_t IndexOf(NodeID v, NodeID target) const;
  bool HasEdge(NodeID u, NodeID v) const;
  NodeID FaceThirdVertex(NodeID u, NodeID v) const;
  std::vector<std::vector<NodeID>> EnumerateFaces() const;
  void InsertNeighborAt(NodeID v, size_t index, NodeID neighbor);

private:
  G *m_graph;
  DenseComponentPool<PlanarRotationAdjacencyComponent> *m_rotation;
};

#include "layout/planarity/rotation_system.tpp"
