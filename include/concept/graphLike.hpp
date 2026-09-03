#include <concepts>
#include <cstdint>
#include <ranges>
#include <utility>

using EntityID = uint32_t;

template <typename G>
concept GraphLike = requires(G graph, const G cgraph, EntityID id) {
  { graph.Vertices() } -> std::ranges::input_range;
  { graph.Neighbors(id) } -> std::ranges::input_range;

  requires std::same_as<std::ranges::range_value_t<decltype(graph.Vertices())>,
                        EntityID>;

  requires std::same_as<std::ranges::range_value_t<decltype(graph.Neighbors(id))>,
                        EntityID>;

  { cgraph.Size() } -> std::convertible_to<size_t>;
  { cgraph.ContainsVertex(id) } -> std::convertible_to<bool>;
  { cgraph.ContaintsEdge(id) } -> std::convertible_to<bool>;

};
