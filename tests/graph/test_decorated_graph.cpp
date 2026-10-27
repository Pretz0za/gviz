#include "catch_amalgamated.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"

TEST_CASE("Init syncs node/edge spaces one-to-one with the graph",
         "[graph][decorated_graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  g.AddEdge(a, b);

  DecoratedGraph<Graph> dg(g);
  REQUIRE(dg.graph == &g);
  REQUIRE(dg.nodeSpace.Size() == g.Size());
  REQUIRE(dg.edgeSpace.Size() == g.EdgeCount());
  REQUIRE(dg.dimension == 2);
}

TEST_CASE("Resources can be set, fetched, and queried for presence",
         "[graph][decorated_graph]") {
  Graph g;
  DecoratedGraph<Graph> dg(g);
  REQUIRE_FALSE(dg.HasResource<int>());
  REQUIRE(dg.GetResource<int>() == nullptr);

  int &value = dg.SetResource<int>(7);
  REQUIRE(value == 7);
  REQUIRE(dg.HasResource<int>());
  REQUIRE(*dg.GetResource<int>() == 7);
}

TEST_CASE("SetResource does not overwrite an already-set resource",
         "[graph][decorated_graph]") {
  Graph g;
  DecoratedGraph<Graph> dg(g);
  dg.SetResource<int>(1);
  int &second = dg.SetResource<int>(2);
  REQUIRE(second == 1);
  REQUIRE(*dg.GetResource<int>() == 1);
}

TEST_CASE("Separate DecoratedGraphs over the same graph have independent resources",
         "[graph][decorated_graph]") {
  Graph g;
  g.AddNode();

  DecoratedGraph<Graph> a(g);
  DecoratedGraph<Graph> b(g);

  a.SetResource<int>(100);
  REQUIRE_FALSE(b.HasResource<int>());

  b.SetResource<int>(7);
  REQUIRE(*a.GetResource<int>() == 100);
  REQUIRE(*b.GetResource<int>() == 7);
}
