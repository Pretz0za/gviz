#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

#include "ecs/admin.hpp"
#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"
#include "graph/types.hpp"

class Graph {
public:
  Graph();

  NodeID AddNode();
  void RemoveNode(NodeID id);
  bool HasNode(NodeID id) const;

  EdgeID AddEdge(NodeID from, NodeID to);
  EdgeID AddEdge(NodeID from, NodeID to, float weight);
  void RemoveEdge(EdgeID id);
  bool HasEdge(EdgeID id) const;
  NodeID Source(EdgeID id) const;
  NodeID Target(EdgeID id) const;

  const std::vector<AdjEntry> &OutEdges(NodeID id) const;
  const std::vector<AdjEntry> &InEdges(NodeID id) const;
  uint32_t OutDegree(NodeID id) const;
  uint32_t InDegree(NodeID id) const;

  auto Nodes() const {
    auto *pool = m_outAdjPool;
    return std::views::iota(size_t{0}, pool->Size()) |
           std::views::transform([pool](size_t i) {
             return NodeID(pool->Owner(static_cast<uint32_t>(i)));
           });
  }

  auto Edges() const {
    auto *pool = m_edgePool;
    return std::views::iota(size_t{0}, pool->Size()) |
           std::views::transform([pool](size_t i) {
             return EdgeID(pool->Owner(static_cast<uint32_t>(i)));
           });
  }

  uint32_t ToCompact(NodeID id) const;

  uint32_t Size() const;

  Admin &Ecs();

  IndexSpace &NodeSpace();
  IndexSpace &EdgeSpace();

private:
  Admin m_admin;
  IndexSpace *m_nodeSpace;
  IndexSpace *m_edgeSpace;
  DenseComponentPool<InAdjacencyComponent> *m_inAdjPool;
  DenseComponentPool<OutAdjacencyComponent> *m_outAdjPool;
  DenseComponentPool<EdgeComponent> *m_edgePool;
  DenseComponentPool<WeightComponent> *m_weightPool;
};

#include "concept/graphLike.hpp"
static_assert(GraphLike<Graph>);
