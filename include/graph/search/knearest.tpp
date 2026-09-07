#pragma once

#include "ds/bfs_scratch.hpp"
#include "graph/search/knearest.hpp"

template <GraphLike G>
KNearestSearch<G>::KNearestSearch(G &graph) : m_graph(&graph) {
  m_scratch = m_graph->template GetResource<BFSScratch>();
  if (m_scratch == nullptr) {
    m_scratch = &m_graph->template SetResource<BFSScratch>(m_graph->Size());
  }
}

template <GraphLike G>
std::vector<FoundNode> KNearestSearch<G>::Find(NodeID id, uint32_t k) {
  std::vector<FoundNode> knearset{};
  m_scratch->InitNew();
  m_scratch->Push(id, 0);

  while (!m_scratch->Empty()) {
    auto nd = m_scratch->Pop();

    for (const AdjEntry &adj : m_graph->OutNeighbors(nd.node)) {
      DenseNodeID nbrCompact = m_graph->MapToDense(adj.other);
      if (m_scratch->IsVisited(nbrCompact))
        continue;
      m_scratch->Visit(nbrCompact);
      knearset.push_back({adj.other, nd.depth + 1});
      if (knearset.size() >= k)
        return knearset;

      m_scratch->Push(adj.other, nd.depth + 1);
    }
  }

  return knearset;
}

template <GraphLike G>
std::vector<FoundNode> KNearestSearch<G>::Find(NodeID id, uint32_t k,
                                               DenseNodeSet filter) {
  std::vector<FoundNode> knearset{};
  m_scratch->InitNew();
  m_scratch->Push(id, 0);

  while (!m_scratch->Empty()) {
    auto nd = m_scratch->Pop();

    for (const AdjEntry &adj : m_graph->OutNeighbors(nd.node)) {
      DenseNodeID nbrCompact = m_graph->MapToDense(adj.other);
      if (m_scratch->IsVisited(nbrCompact))
        continue;
      m_scratch->Visit(nbrCompact);

      if (filter.Test(nbrCompact)) {
        knearset.push_back({adj.other, nd.depth + 1});
        if (knearset.size() >= k)
          return knearset;
      }

      m_scratch->Push(adj.other, nd.depth + 1);
    }
  }

  return knearset;
}
