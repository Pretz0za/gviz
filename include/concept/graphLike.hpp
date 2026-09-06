#pragma once

#include "ecs/index_space.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include <concepts>
#include <cstdint>
#include <ranges>

template <typename G>
concept GraphLike = requires(G graph, const G cgraph, NodeID nid, EdgeID eid) {
  { graph.Nodes() } -> std::ranges::input_range;
  { graph.InNeighbors(nid) } -> std::ranges::input_range;
  { graph.OutNeighbors(nid) } -> std::ranges::input_range;

  requires std::same_as<std::ranges::range_value_t<decltype(graph.Nodes())>,
                        NodeID>;

  // TODO: look into if these ranges are copying
  requires std::same_as<
      std::ranges::range_value_t<decltype(graph.InNeighbors(nid))>, AdjEntry>;
  requires std::same_as<
      std::ranges::range_value_t<decltype(graph.OutNeighbors(nid))>, AdjEntry>;

  { cgraph.Size() } -> std::convertible_to<uint32_t>;

  { cgraph.HasNode(nid) } -> std::convertible_to<bool>;
  { cgraph.HasEdge(eid) } -> std::convertible_to<bool>;
  { cgraph.MapToSparse(DenseNodeID{}) } -> std::same_as<NodeID>;
  { cgraph.MapToDense(nid) } -> std::same_as<DenseNodeID>;
  { graph.NodeSpace() } -> std::same_as<IndexSpace &>;
};
