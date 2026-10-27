#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

#include "ds/flags.hpp"
#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/degree.hpp"
#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"
#include "graph/types.hpp"

class Graph {
public:
  enum class GraphFlags : uint8_t { None = 0, Directed = 1 << 0 };

  Graph();

  NodeID AddNode();
  bool HasNode(NodeID id) const;

  EdgeID AddEdge(NodeID from, NodeID to);
  EdgeID AddEdge(NodeID from, NodeID to, float weight);
  std::pair<EdgeID, EdgeID> AddUndirectedEdge(NodeID a, NodeID b);
  std::pair<EdgeID, EdgeID> AddUndirectedEdge(NodeID a, NodeID b, float weight);
  bool HasEdge(EdgeID id) const;
  EdgeComponent GetEdge(EdgeID id) const;
  float GetWeight(EdgeID id) const;

  const std::vector<AdjEntry> &OutNeighbors(NodeID id) const;
  const std::vector<AdjEntry> &InNeighbors(NodeID id) const;

  uint32_t OutDegree(NodeID id) const;
  uint32_t InDegree(NodeID id) const;
  uint32_t Degree(NodeID id) const;

  auto Nodes() const {
    return std::views::iota(uint32_t{0},
                            static_cast<uint32_t>(m_nodeSpace.Size())) |
           std::views::transform([](uint32_t i) { return NodeID(i); });
  }

  auto Edges() const {
    return std::views::iota(uint32_t{0},
                            static_cast<uint32_t>(m_edgeSpace.Size())) |
           std::views::transform([](uint32_t i) { return EdgeID(i); });
  }

  constexpr NodeID MapToSparse(DenseNodeID id) const {
    return NodeID(id.Raw());
  };
  constexpr DenseNodeID MapToDense(NodeID id) const {
    return DenseNodeID(id.Raw());
  };

  constexpr EdgeID MapToSparse(DenseEdgeID id) const {
    return EdgeID(id.Raw());
  };
  constexpr DenseEdgeID MapToDense(EdgeID id) const {
    return DenseEdgeID(id.Raw());
  };

  uint32_t Size() const;
  uint32_t EdgeCount() const { return m_edgeSpace.Size(); }

  // Increases on every AddNode/AddEdge call, so callers can cheaply detect
  // whether the graph's topology has changed since they last looked.
  uint64_t Version() const { return m_version; }

  bool IsDirected() const { return m_flags.Has(GraphFlags::Directed); }

private:
  IndexSpace m_nodeSpace;
  IndexSpace m_edgeSpace;
  DenseComponentPool<InAdjacencyComponent> *m_inAdjPool;
  DenseComponentPool<OutAdjacencyComponent> *m_outAdjPool;
  DenseComponentPool<DegreeComponent> *m_degreePool;
  DenseComponentPool<EdgeComponent> *m_edgePool;
  DenseComponentPool<WeightComponent> *m_weightPool;
  uint64_t m_version = 0;
  Flags<GraphFlags> m_flags = GraphFlags::None;
};

#include "concept/graphLike.hpp"
static_assert(GraphLike<Graph>);
