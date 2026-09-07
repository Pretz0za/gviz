#pragma once

#include "concept/graphLike.hpp"
#include "ds/bfs_scratch.hpp"
#include "graph/search/types.hpp"
#include "graph/types.hpp"
#include <cstdint>
#include <vector>
template <GraphLike G> class KNearestSearch {
public:
  KNearestSearch(G &graph);

  std::vector<FoundNode> Find(NodeID id, uint32_t k);
  std::vector<FoundNode> Find(NodeID id, uint32_t k, DenseNodeSet filter);

private:
  G *m_graph;
  BFSScratch *m_scratch;
};

#include "graph/search/knearest.tpp"
