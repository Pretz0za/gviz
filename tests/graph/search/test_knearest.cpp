// KNearestSearch performs a breadth-first walk from a source node, stopping
// as soon as it has collected k neighbors (or the graph is exhausted). It
// reuses a BFSScratch resource stashed on the graph across calls.

#include "catch_amalgamated.hpp"
#include "graph/graph.hpp"
#include "graph/search/knearest.hpp"

#include <vector>

namespace {
// 0 -> 1 -> 2 -> 3 -> 4
Graph MakeChain(uint32_t length) {
  Graph g;
  std::vector<NodeID> nodes;
  for (uint32_t i = 0; i < length; i++)
    nodes.push_back(g.AddNode());
  for (uint32_t i = 0; i + 1 < length; i++)
    g.AddEdge(nodes[i], nodes[i + 1]);
  return g;
}
} // namespace

TEST_CASE("Find returns immediate neighbors first, in BFS order",
         "[graph][search][knearest]") {
  Graph g = MakeChain(5);
  KNearestSearch<Graph> search(g);

  std::vector<FoundNode> found = search.Find(NodeID(0), 3);

  REQUIRE(found.size() == 3);
  REQUIRE(found[0].node == NodeID(1));
  REQUIRE(found[0].depth == 1);
  REQUIRE(found[1].node == NodeID(2));
  REQUIRE(found[1].depth == 2);
  REQUIRE(found[2].node == NodeID(3));
  REQUIRE(found[2].depth == 3);
}

TEST_CASE("Find stops early once k results are collected", "[graph][search][knearest]") {
  Graph g = MakeChain(10);
  KNearestSearch<Graph> search(g);

  std::vector<FoundNode> found = search.Find(NodeID(0), 1);
  REQUIRE(found.size() == 1);
  REQUIRE(found[0].node == NodeID(1));
}

TEST_CASE("Find returns fewer than k results when the graph is exhausted",
         "[graph][search][knearest]") {
  Graph g = MakeChain(3); // node 2 has no outgoing edges
  KNearestSearch<Graph> search(g);

  std::vector<FoundNode> found = search.Find(NodeID(0), 100);
  REQUIRE(found.size() == 2);
}

TEST_CASE("Find on an isolated node returns nothing", "[graph][search][knearest]") {
  Graph g;
  g.AddNode();
  KNearestSearch<Graph> search(g);

  std::vector<FoundNode> found = search.Find(NodeID(0), 5);
  REQUIRE(found.empty());
}

TEST_CASE("A node is never visited/returned twice, even with converging paths",
         "[graph][search][knearest]") {
  // 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3 (diamond)
  Graph g;
  NodeID n0 = g.AddNode();
  NodeID n1 = g.AddNode();
  NodeID n2 = g.AddNode();
  NodeID n3 = g.AddNode();
  g.AddEdge(n0, n1);
  g.AddEdge(n0, n2);
  g.AddEdge(n1, n3);
  g.AddEdge(n2, n3);

  KNearestSearch<Graph> search(g);
  std::vector<FoundNode> found = search.Find(n0, 100);

  REQUIRE(found.size() == 3); // n1, n2, n3 -- n3 only counted once
  int n3Count = 0;
  for (const auto &fn : found)
    if (fn.node == n3)
      n3Count++;
  REQUIRE(n3Count == 1);
}

TEST_CASE("The filtered overload only returns nodes present in the filter, "
         "but still traverses through nodes that aren't",
         "[graph][search][knearest]") {
  Graph g = MakeChain(5); // 0 -> 1 -> 2 -> 3 -> 4
  KNearestSearch<Graph> search(g);

  DenseNodeSet filter(5, 0);
  filter.Set(DenseNodeID(3)); // only node 3 is "interesting"

  std::vector<FoundNode> found = search.Find(NodeID(0), 1, filter);

  REQUIRE(found.size() == 1);
  REQUIRE(found[0].node == NodeID(3));
  REQUIRE(found[0].depth == 3);
}

TEST_CASE("The filtered overload returns nothing if no reachable node matches",
         "[graph][search][knearest]") {
  Graph g = MakeChain(3);
  KNearestSearch<Graph> search(g);

  DenseNodeSet filter(3, 0); // nothing marked

  std::vector<FoundNode> found = search.Find(NodeID(0), 5, filter);
  REQUIRE(found.empty());
}

TEST_CASE("Repeated calls on the same KNearestSearch reset cleanly between "
         "searches",
         "[graph][search][knearest]") {
  Graph g = MakeChain(4);
  KNearestSearch<Graph> search(g);

  std::vector<FoundNode> first = search.Find(NodeID(0), 100);
  std::vector<FoundNode> second = search.Find(NodeID(0), 100);

  REQUIRE(first.size() == second.size());
  for (size_t i = 0; i < first.size(); i++) {
    REQUIRE(first[i].node == second[i].node);
    REQUIRE(first[i].depth == second[i].depth);
  }
}
