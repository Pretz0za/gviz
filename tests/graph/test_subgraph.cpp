// Unit tests for Subgraph, a filtered view over a subset of a parent
// Graph's nodes (and, transitively, only the edges whose endpoints are both
// in that subset).
//
// Subgraph sizes its internal maps to the parent's Size() at construction
// time, so -- matching the class's implicit contract -- every test here
// builds the full parent graph before constructing the Subgraph.

#include "catch_amalgamated.hpp"
#include "graph/graph.hpp"
#include "graph/subgraph.hpp"

#include <vector>

TEST_CASE("A fresh Subgraph contains none of the parent's nodes", "[graph][subgraph]") {
  Graph g;
  g.AddNode();
  g.AddNode();

  Subgraph sub(g);
  REQUIRE(sub.Size() == 0);
  REQUIRE_FALSE(sub.HasNode(NodeID(0)));
  REQUIRE_FALSE(sub.HasNode(NodeID(1)));
}

TEST_CASE("AddNode makes a parent node visible in the subgraph", "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  g.AddNode();

  Subgraph sub(g);
  sub.AddNode(a);

  REQUIRE(sub.Size() == 1);
  REQUIRE(sub.HasNode(a));
  REQUIRE_FALSE(sub.HasNode(NodeID(1)));
}

TEST_CASE("MapToDense/MapToSparse agree for nodes added to the subgraph",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  g.AddNode();

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  DenseNodeID denseA = sub.MapToDense(a);
  DenseNodeID denseB = sub.MapToDense(b);

  REQUIRE(denseA.Raw() == 0u);
  REQUIRE(denseB.Raw() == 1u);
  REQUIRE(sub.MapToSparse(denseA) == a);
  REQUIRE(sub.MapToSparse(denseB) == b);
}

TEST_CASE("OutNeighbors/InNeighbors only include neighbors also in the subgraph",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode(); // will be excluded from the subgraph
  g.AddEdge(a, b);
  g.AddEdge(a, c);

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  std::vector<NodeID> outNeighbors;
  for (const auto &adj : sub.OutNeighbors(a))
    outNeighbors.push_back(adj.other);

  REQUIRE(outNeighbors.size() == 1);
  REQUIRE(outNeighbors[0] == b);
}

TEST_CASE("HasEdge is true only when both endpoints are present in the subgraph",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode();
  EdgeID ab = g.AddEdge(a, b);
  EdgeID ac = g.AddEdge(a, c);

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  REQUIRE(sub.HasEdge(ab));
  REQUIRE_FALSE(sub.HasEdge(ac));
}

TEST_CASE("GetEdge returns INVALID_EDGE when an endpoint is missing from the "
         "subgraph",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode();
  EdgeID ac = g.AddEdge(a, c);

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  REQUIRE(sub.GetEdge(ac) == INVALID_EDGE);
}

TEST_CASE("Edges() filters the parent's edges down to fully-visible ones",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode();
  EdgeID ab = g.AddEdge(a, b);
  g.AddEdge(a, c);

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  std::vector<EdgeID> edges;
  for (EdgeID e : sub.Edges())
    edges.push_back(e);

  REQUIRE(edges.size() == 1);
  REQUIRE(edges[0] == ab);
}

TEST_CASE("Degree counts within the subgraph ignore edges to excluded nodes",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode();
  g.AddEdge(a, b);
  g.AddEdge(a, c);
  g.AddEdge(b, a);

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  REQUIRE(sub.OutDegree(a) == 1); // a->b only; a->c excluded
  REQUIRE(sub.InDegree(a) == 1);  // b->a
  REQUIRE(sub.Degree(a) == 2);
}

TEST_CASE("Nodes() enumerates exactly the nodes added to the subgraph",
         "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  g.AddNode();

  Subgraph sub(g);
  sub.AddNode(a);
  sub.AddNode(b);

  std::vector<NodeID> nodes(sub.Nodes().begin(), sub.Nodes().end());
  REQUIRE(nodes.size() == 2);
}

TEST_CASE("Version tracks how many nodes have been added", "[graph][subgraph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();

  Subgraph sub(g);
  REQUIRE(sub.Version() == 0);
  sub.AddNode(a);
  REQUIRE(sub.Version() == 1);
  sub.AddNode(b);
  REQUIRE(sub.Version() == 2);
}

TEST_CASE("Subgraph resources are independent of the parent graph's resources",
         "[graph][subgraph]") {
  Graph g;
  g.AddNode();
  g.SetResource<int>(100);

  Subgraph sub(g);
  REQUIRE_FALSE(sub.HasResource<int>());

  sub.SetResource<int>(7);
  REQUIRE(*sub.GetResource<int>() == 7);
  REQUIRE(*g.GetResource<int>() == 100);
}
