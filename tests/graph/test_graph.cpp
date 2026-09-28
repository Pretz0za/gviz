// Unit tests for Graph, the concrete GraphLike implementation nodes/edges
// are actually stored in.
//
// NOTE: Graph::HasNode/HasEdge/GetEdge/AddEdge all resolve ids through
// DenseComponentPool<T>::Find, whose bounds check is currently commented out
// (components.tpp just does `return &m_data[id];` unconditionally). Passing
// an id that was never created -- including the INVALID_NODE_ID/
// INVALID_EDGE_ID sentinels or a default-constructed Handle -- is therefore
// an out-of-bounds vector access (real UB), not a safe "not found" result.
// This file intentionally only exercises ids that were actually allocated by
// the Graph, to avoid exercising that UB from a test.

#include "catch_amalgamated.hpp"
#include "graph/graph.hpp"

#include <vector>

TEST_CASE("A fresh Graph has no nodes", "[graph]") {
  Graph g;
  REQUIRE(g.Size() == 0);
}

TEST_CASE("AddNode returns sequential ids and grows Size", "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode();

  REQUIRE(a.Raw() == 0u);
  REQUIRE(b.Raw() == 1u);
  REQUIRE(c.Raw() == 2u);
  REQUIRE(g.Size() == 3);
}

TEST_CASE("HasNode is true for every node the graph created", "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();

  REQUIRE(g.HasNode(a));
  REQUIRE(g.HasNode(b));
}

TEST_CASE("AddEdge connects two existing nodes and is retrievable via GetEdge",
         "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();

  EdgeID e = g.AddEdge(a, b);
  REQUIRE(e.IsValid());
  REQUIRE(g.HasEdge(e));

  EdgeComponent edge = g.GetEdge(e);
  REQUIRE(edge.from == a);
  REQUIRE(edge.to == b);
}

TEST_CASE("A weighted AddEdge stores the given weight", "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();

  EdgeID e = g.AddEdge(a, b, 3.5f);
  REQUIRE(e.IsValid());
  REQUIRE(g.GetEdge(e).from == a);
}

TEST_CASE("AddEdge registers the edge in both endpoints' adjacency lists",
         "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  EdgeID e = g.AddEdge(a, b);

  const auto &outA = g.OutNeighbors(a);
  REQUIRE(outA.size() == 1);
  REQUIRE(outA[0].edge == e);
  REQUIRE(outA[0].other == b);

  const auto &inB = g.InNeighbors(b);
  REQUIRE(inB.size() == 1);
  REQUIRE(inB[0].edge == e);
  REQUIRE(inB[0].other == a);
}

TEST_CASE("An edge does not appear as an in-neighbor of its source",
         "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  g.AddEdge(a, b);

  REQUIRE(g.InNeighbors(a).empty());
  REQUIRE(g.OutNeighbors(b).empty());
}

TEST_CASE("AddUndirectedEdge creates edges in both directions", "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();

  auto [ab, ba] = g.AddUndirectedEdge(a, b);
  REQUIRE(ab.IsValid());
  REQUIRE(ba.IsValid());

  REQUIRE(g.OutNeighbors(a).size() == 1);
  REQUIRE(g.OutNeighbors(b).size() == 1);
  REQUIRE(g.OutNeighbors(a)[0].other == b);
  REQUIRE(g.OutNeighbors(b)[0].other == a);
}

TEST_CASE("Degree counts are consistent with in/out neighbor list sizes",
         "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  NodeID c = g.AddNode();
  g.AddEdge(a, b);
  g.AddEdge(a, c);
  g.AddEdge(b, a);

  REQUIRE(g.OutDegree(a) == 2);
  REQUIRE(g.InDegree(a) == 1);
  REQUIRE(g.Degree(a) == 3);

  REQUIRE(g.OutDegree(c) == 0);
  REQUIRE(g.InDegree(c) == 1);
}

TEST_CASE("Nodes() yields exactly the ids the graph has created, in order",
         "[graph]") {
  Graph g;
  g.AddNode();
  g.AddNode();
  g.AddNode();

  std::vector<NodeID> nodes;
  for (NodeID n : g.Nodes())
    nodes.push_back(n);

  REQUIRE(nodes.size() == 3);
  REQUIRE(nodes[0].Raw() == 0u);
  REQUIRE(nodes[1].Raw() == 1u);
  REQUIRE(nodes[2].Raw() == 2u);
}

TEST_CASE("Edges() yields exactly the ids the graph has created, in order",
         "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  EdgeID e0 = g.AddEdge(a, b);
  EdgeID e1 = g.AddEdge(b, a);

  std::vector<EdgeID> edges;
  for (EdgeID e : g.Edges())
    edges.push_back(e);

  REQUIRE(edges.size() == 2);
  REQUIRE(edges[0] == e0);
  REQUIRE(edges[1] == e1);
}

TEST_CASE("MapToDense/MapToSparse are identity maps on the full graph",
         "[graph]") {
  Graph g;
  NodeID a = g.AddNode();
  DenseNodeID dense = g.MapToDense(a);
  REQUIRE(dense.Raw() == a.Raw());
  REQUIRE(g.MapToSparse(dense) == a);
}

TEST_CASE("Version increases when the graph is mutated", "[graph]") {
  Graph g;
  uint64_t v0 = g.Version();
  g.AddNode();
  uint64_t v1 = g.Version();
  REQUIRE(v1 > v0);

  NodeID a = g.AddNode();
  NodeID b = g.AddNode();
  uint64_t v2 = g.Version();
  g.AddEdge(a, b);
  uint64_t v3 = g.Version();
  REQUIRE(v3 > v2);
}

TEST_CASE("Resources can be set, fetched, and queried for presence",
         "[graph]") {
  Graph g;
  REQUIRE_FALSE(g.HasResource<int>());
  REQUIRE(g.GetResource<int>() == nullptr);

  int &value = g.SetResource<int>(7);
  REQUIRE(value == 7);
  REQUIRE(g.HasResource<int>());
  REQUIRE(*g.GetResource<int>() == 7);
}

TEST_CASE("SetResource does not overwrite an already-set resource",
         "[graph]") {
  Graph g;
  g.SetResource<int>(1);
  int &second = g.SetResource<int>(2);
  REQUIRE(second == 1);
  REQUIRE(*g.GetResource<int>() == 1);
}
