#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/search/knearest.hpp"
#include "graph/search/types.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/types.hpp"
#include <vector>

template <GraphLike G> class PositionBarrycentric {
public:
  PositionBarrycentric(G &graph);

  // NOTE: this currently only iterates out neighbors.
  void PlaceBetweenNeighbors(NodeID id);
  void PlaceBetweenKNearest(NodeID id, uint32_t k);
  void PlaceBetweenKNearest(NodeID id, uint32_t k, DenseNodeSet &filter);

private:
  void placeBetweenFound(NodeID id, std::vector<FoundNode> found);

  G *m_graph;
  uint8_t m_dimension;
  DenseComponentPool<PositionComponent> *m_positions;
  KNearestSearch<G> m_knnSystem;
};

#include "layout/placement/barrycenter.tpp"
