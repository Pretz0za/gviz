#pragma once

#include "ecs/index_space.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"
#include <concepts>
#include <cstdint>
#include <ranges>

namespace detail {
// Stand-in type used only to prove GraphLike's resource accessors are
// generic member templates, without the concept naming any real resource
// type (e.g. DimensionResource).
struct GraphLikeResourceProbe {};
} // namespace detail

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

  { cgraph.OutDegree(nid) } -> std::convertible_to<uint32_t>;
  { cgraph.Degree(nid) } -> std::convertible_to<uint32_t>;
  { cgraph.InDegree(nid) } -> std::convertible_to<uint32_t>;
  { cgraph.HasNode(nid) } -> std::convertible_to<bool>;
  { cgraph.HasEdge(eid) } -> std::convertible_to<bool>;
  { cgraph.GetEdge(eid) } -> std::same_as<EdgeComponent>;
  { cgraph.MapToSparse(DenseNodeID{}) } -> std::same_as<NodeID>;
  { cgraph.MapToDense(nid) } -> std::same_as<DenseNodeID>;
  { graph.NodeSpace() } -> std::same_as<IndexSpace &>;
  { cgraph.Version() } -> std::convertible_to<uint64_t>;

  // Generic resource-template interface (see GraphLikeResourceProbe above).
  { graph.template SetResource<detail::GraphLikeResourceProbe>() };
  {
    graph.template GetResource<detail::GraphLikeResourceProbe>()
  } -> std::same_as<detail::GraphLikeResourceProbe *>;
  {
    cgraph.template HasResource<detail::GraphLikeResourceProbe>()
  } -> std::convertible_to<bool>;
};
