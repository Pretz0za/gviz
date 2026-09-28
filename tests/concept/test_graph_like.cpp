// GraphLike is a compile-time contract; Graph and Subgraph already static_assert
// against it in their own headers, but this file exercises the concept
// directly so a future GraphLike-modeling type has a place to add the same
// check, and so failures here point straight at the concept file itself.

#include "catch_amalgamated.hpp"
#include "concept/graphLike.hpp"
#include "graph/graph.hpp"
#include "graph/subgraph.hpp"

TEST_CASE("Graph and Subgraph satisfy GraphLike", "[concept][graph_like]") {
  STATIC_REQUIRE(GraphLike<Graph>);
  STATIC_REQUIRE(GraphLike<Subgraph>);
}

TEST_CASE("An unrelated type does not satisfy GraphLike", "[concept][graph_like]") {
  struct NotAGraph {};
  STATIC_REQUIRE_FALSE(GraphLike<NotAGraph>);
}
