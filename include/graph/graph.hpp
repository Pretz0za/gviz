#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include "ecs/resource.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"
#include "graph/types.hpp"

class Graph {
public:
  Graph();

  NodeID AddNode();
  bool HasNode(NodeID id) const;

  EdgeID AddEdge(NodeID from, NodeID to);
  EdgeID AddEdge(NodeID from, NodeID to, float weight);
  std::pair<EdgeID, EdgeID> AddUndirectedEdge(NodeID a, NodeID b);
  std::pair<EdgeID, EdgeID> AddUndirectedEdge(NodeID a, NodeID b, float weight);
  bool HasEdge(EdgeID id) const;
  EdgeComponent GetEdge(EdgeID id) const;

  const std::vector<AdjEntry> &OutNeighbors(NodeID id) const;
  const std::vector<AdjEntry> &InNeighbors(NodeID id) const;
  uint32_t OutDegree(NodeID id) const;
  uint32_t InDegree(NodeID id) const;

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

  constexpr NodeID MapToSparse(NodeID id) const { return id; };
  constexpr NodeID MapToDense(NodeID id) const { return id; };

  uint32_t Size() const;

  IndexSpace &NodeSpace();
  IndexSpace &EdgeSpace();

  template <typename T, typename... Args> T &SetResource(Args &&...args);
  template <typename T> T *GetResource();
  template <typename T> bool HasResource() const;

private:
  IndexSpace m_nodeSpace;
  IndexSpace m_edgeSpace;
  DenseComponentPool<InAdjacencyComponent> *m_inAdjPool;
  DenseComponentPool<OutAdjacencyComponent> *m_outAdjPool;
  DenseComponentPool<EdgeComponent> *m_edgePool;
  DenseComponentPool<WeightComponent> *m_weightPool;
  std::unordered_map<std::type_index, std::unique_ptr<IResourceHolder>>
      m_resources;
};

#include "graph/graph.tpp"

#include "concept/graphLike.hpp"
static_assert(GraphLike<Graph>);
