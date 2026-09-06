#pragma once

#include "ds/bitset.hpp"
#include "ecs/index_space.hpp"
#include "graph/components/edge.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include <cstdint>
#include <ranges>
#include <vector>

class Subgraph {
public:
  Subgraph(Graph &parent);

  void AddNode(NodeID);

  bool HasNode(NodeID id) const;
  bool HasEdge(EdgeID id) const;

  EdgeComponent GetEdge(EdgeID id) const;

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

  auto Nodes() const {
    return m_nodeSet | std::views::transform([](size_t i) {
             return NodeID(static_cast<EntityID>(i));
           });
  }

  // auto Edges() const;

  constexpr DenseNodeID MapToDense(NodeID id) const {
    return m_mapToDense[id.Raw()];
  };

  constexpr NodeID MapToSparse(DenseNodeID id) const {
	return m_mapToSparse[id.Raw()];
  }

  uint32_t Size() const { return m_size; };

  IndexSpace &NodeSpace();

  // NOTE: maybe this could be removed

  // IndexSpace &EdgeSpace();

  template <typename T, typename... Args> T &SetResource(Args &&...args);
  template <typename T> T *GetResource();
  template <typename T> bool HasResource() const;

private:
  uint32_t m_size = 0;
  IndexSpace m_compactNodeSpace;
  Graph *m_parent;
  BitSet m_nodeSet;
  std::unordered_map<std::type_index, std::unique_ptr<IResourceHolder>>
      m_resources;

  std::vector<DenseNodeID>
      m_mapToDense; // parent graph index -> compact index in subgraph's pools
  std::vector<NodeID> m_mapToSparse; // compact index -> parent graph index
};

#include "graph/subgraph.tpp"

#include "concept/graphLike.hpp"
static_assert(GraphLike<Subgraph>);
