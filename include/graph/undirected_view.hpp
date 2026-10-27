#pragma once

#include "concept/graphLike.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include <cstdint>
#include <generator>

template <GraphLike G> class UndirectedView {
public:
  explicit UndirectedView(G *graph) : m_graph(graph) {}

  auto Nodes() const { return m_graph->Nodes(); }
  auto Edges() const { return m_graph->Edges(); }

  bool HasNode(NodeID id) const { return m_graph->HasNode(id); }
  bool HasEdge(EdgeID id) const { return m_graph->HasEdge(id); }
  EdgeComponent GetEdge(EdgeID id) const { return m_graph->GetEdge(id); }
  float GetWeight(EdgeID id) const { return m_graph->GetWeight(id); }

  std::generator<AdjEntry> OutNeighbors(NodeID id) const {
    if (!m_graph->IsDirected()) {
      for (const auto &e : m_graph->OutNeighbors(id))
        co_yield e;
      co_return;
    }
    for (auto &&e : CombinedNeighbors(id))
      co_yield e;
  }

  std::generator<AdjEntry> InNeighbors(NodeID id) const {
    if (!m_graph->IsDirected()) {
      for (const auto &e : m_graph->InNeighbors(id))
        co_yield e;
      co_return;
    }
    for (auto &&e : CombinedNeighbors(id))
      co_yield e;
  }

  uint32_t OutDegree(NodeID id) const { return UndirectedDegree(id); }
  uint32_t InDegree(NodeID id) const { return UndirectedDegree(id); }
  uint32_t Degree(NodeID id) const { return UndirectedDegree(id); }

  uint32_t Size() const { return m_graph->Size(); }
  uint32_t EdgeCount() const { return m_graph->EdgeCount(); }

  bool IsDirected() const { return false; }

  NodeID MapToSparse(DenseNodeID id) const { return m_graph->MapToSparse(id); }
  DenseNodeID MapToDense(NodeID id) const { return m_graph->MapToDense(id); }
  EdgeID MapToSparse(DenseEdgeID id) const { return m_graph->MapToSparse(id); }
  DenseEdgeID MapToDense(EdgeID id) const { return m_graph->MapToDense(id); }

private:
  std::generator<AdjEntry> CombinedNeighbors(NodeID id) const {
    for (const auto &e : m_graph->InNeighbors(id))
      co_yield e;
    for (const auto &e : m_graph->OutNeighbors(id))
      co_yield e;
  }

  uint32_t UndirectedDegree(NodeID id) const {
    if (!m_graph->IsDirected())
      return m_graph->OutDegree(id);
    return m_graph->InDegree(id) + m_graph->OutDegree(id);
  }

  G *m_graph;
};

static_assert(GraphLike<UndirectedView<Graph>>);
