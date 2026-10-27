#pragma once

#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include "graph/components/degree.hpp"
#include "graph/components/edge.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include <cstdint>
#include <ranges>

class Subgraph {
public:
  Subgraph(Graph &parent);

  DenseNodeID AddNode(NodeID);

  inline bool HasNode(NodeID id) const { return m_nodeSet.Test(id); }
  inline bool HasEdge(EdgeID id) const { return m_edgeSet.Test(id); }

  EdgeComponent GetEdge(EdgeID id) const;
  float GetWeight(EdgeID id) const { return m_parent->GetWeight(id); }

  auto OutNeighbors(NodeID id) const {
    return m_parent->OutNeighbors(id) |
           std::views::filter(
               [this](const AdjEntry &e) { return HasNode(e.other); });
  }

  auto InNeighbors(NodeID id) const {
    return m_parent->InNeighbors(id) |
           std::views::filter(
               [this](const AdjEntry &e) { return HasNode(e.other); });
  }

  uint32_t OutDegree(NodeID id) const;
  uint32_t InDegree(NodeID id) const;
  uint32_t Degree(NodeID id) const;

  auto Nodes() const {
    return std::ranges::subrange(m_nodeSet.begin(), m_nodeSet.end());
  }

  auto Edges() const {
    return m_parent->Edges() |
           std::views::filter([this](EdgeID id) { return HasEdge(id); });
  }

  constexpr DenseNodeID MapToDense(NodeID id) const {
    return DenseNodeID{m_compactNodeSpace.MapToDense(id.Raw())};
  };

  constexpr NodeID MapToSparse(DenseNodeID id) const {
    return NodeID{m_compactNodeSpace.MapToSparse(id.Raw())};
  }

  constexpr DenseEdgeID MapToDense(EdgeID id) const {
    return DenseEdgeID{m_compactEdgeSpace.MapToDense(id.Raw())};
  };

  constexpr EdgeID MapToSparse(DenseEdgeID id) const {
    return EdgeID{m_compactEdgeSpace.MapToSparse(id.Raw())};
  }

  uint32_t Size() const { return m_size; };
  uint32_t EdgeCount() const { return m_compactEdgeSpace.Size(); };

  // How many nodes have been added to this subgraph so far.
  uint32_t Version() const { return m_size; }

  bool IsDirected() const { return m_parent->IsDirected(); }

private:
  void setEdgesAndDegrees(NodeID id);

  uint32_t m_size = 0;
  Graph *m_parent;
  IndexSpace m_compactEdgeSpace;
  IndexSpace m_compactNodeSpace;
  SparseNodeSet m_nodeSet;
  SparseEdgeSet m_edgeSet;
  DenseComponentPool<DegreeComponent> *m_degrees;
};

#include "concept/graphLike.hpp"
static_assert(GraphLike<Subgraph>);
