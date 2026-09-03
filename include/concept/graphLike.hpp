#pragma once

#include "ecs/admin.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include <concepts>
#include <cstdint>
#include <ranges>

template <typename G>
concept GraphLike = requires(G graph, const G cgraph, NodeID nid, EdgeID eid) {
  { graph.Nodes() } -> std::ranges::input_range;
  { graph.InEdges(nid) } -> std::ranges::input_range;
  { graph.OutEdges(nid) } -> std::ranges::input_range;

  requires std::same_as<std::ranges::range_value_t<decltype(graph.Nodes())>,
                        NodeID>;

  requires std::same_as<std::ranges::range_value_t<decltype(graph.InEdges(nid))>,
                        AdjEntry>;
  requires std::same_as<std::ranges::range_value_t<decltype(graph.OutEdges(nid))>,
                        AdjEntry>;

  { cgraph.Size() } -> std::convertible_to<uint32_t>;
  { cgraph.HasNode(nid) } -> std::convertible_to<bool>;
  { cgraph.HasEdge(eid) } -> std::convertible_to<bool>;
  { graph.Ecs() } -> std::same_as<Admin&>;

};
