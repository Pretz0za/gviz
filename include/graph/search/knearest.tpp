#pragma once

#include "ds/bfs_scratch.hpp"
#include "graph/search/knearest.hpp"

template <GraphLike G>
KNearestSearch<G>::KNearestSearch(DecoratedGraph<G> &dg) : m_graph(dg.graph) {
  m_scratch = dg.template GetResource<BFSScratch>();
  if (m_scratch == nullptr) {
    m_scratch = &dg.template SetResource<BFSScratch>(m_graph->Size());
  }
}

template <GraphLike G>
std::vector<FoundNode> KNearestSearch<G>::Find(NodeID id, uint32_t k) {
  std::vector<FoundNode> knearset{};
  m_scratch->InitNew();
  m_scratch->Visit(m_graph->MapToDense(id));
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
  m_scratch->Visit(m_graph->MapToDense(id));
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
