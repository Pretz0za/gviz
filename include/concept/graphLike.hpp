#pragma once

#include "ecs/index_space.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"
#include <concepts>
#include <cstdint>
#include <ranges>

template <typename G>
concept GraphLike = requires(G graph, const G cgraph, NodeID nid, EdgeID eid) {
  { graph.Nodes() } -> std::ranges::input_range;
  { graph.Edges() } -> std::ranges::input_range;
  { graph.InNeighbors(nid) } -> std::ranges::input_range;
  { graph.OutNeighbors(nid) } -> std::ranges::input_range;

  requires std::same_as<std::ranges::range_value_t<decltype(graph.Nodes())>,
                        NodeID>;
  requires std::same_as<std::ranges::range_value_t<decltype(graph.Edges())>,
                        EdgeID>;

  // TODO: look into if these ranges are copying
  requires std::same_as<
      std::ranges::range_value_t<decltype(graph.InNeighbors(nid))>, AdjEntry>;
  requires std::same_as<
      std::ranges::range_value_t<decltype(graph.OutNeighbors(nid))>, AdjEntry>;

  { cgraph.Size() } -> std::convertible_to<uint32_t>;
  { cgraph.EdgeCount() } -> std::convertible_to<uint32_t>;

  { cgraph.IsDirected() } -> std::convertible_to<bool>;

  { cgraph.OutDegree(nid) } -> std::convertible_to<uint32_t>;
  { cgraph.Degree(nid) } -> std::convertible_to<uint32_t>;
  { cgraph.InDegree(nid) } -> std::convertible_to<uint32_t>;
  { cgraph.HasNode(nid) } -> std::convertible_to<bool>;
  { cgraph.HasEdge(eid) } -> std::convertible_to<bool>;
  { cgraph.GetEdge(eid) } -> std::same_as<EdgeComponent>;
  { cgraph.GetWeight(eid) } -> std::convertible_to<float>;
  { cgraph.MapToSparse(DenseNodeID{}) } -> std::same_as<NodeID>;
  { cgraph.MapToDense(nid) } -> std::same_as<DenseNodeID>;
  { cgraph.MapToSparse(DenseEdgeID{}) } -> std::same_as<EdgeID>;
  { cgraph.MapToDense(eid) } -> std::same_as<DenseEdgeID>;
};
