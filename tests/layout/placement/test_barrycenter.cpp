// PositionBarrycentric places a node at the average position of some
// neighborhood: its direct out-neighbors, or its k-nearest BFS neighbors
// (optionally filtered to a subset of "visible" nodes).

#include "catch_amalgamated.hpp"
#include "layout/placement/barrycenter.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::SetPosition;
using test_helpers::WithPositions;

TEST_CASE("PlaceBetweenNeighbors averages the positions of out-neighbors",
         "[layout][placement][barrycenter]") {
  Graph g = MakeGraph(3, 2);
  auto *positions = WithPositions(g, 2);
  g.AddEdge(NodeID(0), NodeID(1));
  g.AddEdge(NodeID(0), NodeID(2));

  SetPosition(positions, 1, {2.0, 4.0});
  SetPosition(positions, 2, {6.0, 8.0});

  PositionBarrycentric<Graph> placer(g);
  placer.PlaceBetweenNeighbors(NodeID(0));

  PositionComponent *p0 = positions->Find(0);
  REQUIRE(p0->pos[0] == Catch::Approx(4.0));
  REQUIRE(p0->pos[1] == Catch::Approx(6.0));
}

TEST_CASE("PlaceBetweenNeighbors with a single neighbor copies that neighbor's position",
         "[layout][placement][barrycenter]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  g.AddEdge(NodeID(0), NodeID(1));
  SetPosition(positions, 1, {7.0, -3.0});

  PositionBarrycentric<Graph> placer(g);
  placer.PlaceBetweenNeighbors(NodeID(0));

  PositionComponent *p0 = positions->Find(0);
  REQUIRE(p0->pos[0] == Catch::Approx(7.0));
  REQUIRE(p0->pos[1] == Catch::Approx(-3.0));
}

TEST_CASE("PlaceBetweenKNearest averages the k nearest BFS-discovered nodes",
         "[layout][placement][barrycenter]") {
  // 0 -> 1 -> 2 -> 3
  Graph g = MakeGraph(4, 2);
  auto *positions = WithPositions(g, 2);
  g.AddEdge(NodeID(0), NodeID(1));
  g.AddEdge(NodeID(1), NodeID(2));
  g.AddEdge(NodeID(2), NodeID(3));

  SetPosition(positions, 1, {10.0, 0.0});
  SetPosition(positions, 2, {20.0, 0.0});

  PositionBarrycentric<Graph> placer(g);
  placer.PlaceBetweenKNearest(NodeID(0), 2);

  // BFS from 0 with k=2 finds nodes 1 and 2 (depth 1, 2); average is (15, 0).
  PositionComponent *p0 = positions->Find(0);
  REQUIRE(p0->pos[0] == Catch::Approx(15.0));
  REQUIRE(p0->pos[1] == Catch::Approx(0.0));
}

TEST_CASE("PlaceBetweenKNearest with a filter only averages nodes marked visible",
         "[layout][placement][barrycenter]") {
  // 0 -> 1 -> 2 -> 3
  Graph g = MakeGraph(4, 2);
  auto *positions = WithPositions(g, 2);
  g.AddEdge(NodeID(0), NodeID(1));
  g.AddEdge(NodeID(1), NodeID(2));
  g.AddEdge(NodeID(2), NodeID(3));

  SetPosition(positions, 1, {10.0, 0.0});
  SetPosition(positions, 2, {20.0, 0.0});
  SetPosition(positions, 3, {30.0, 0.0});

  DenseNodeSet filter(4, 0);
  filter.Set(DenseNodeID(3)); // only node 3 is "interesting"

  PositionBarrycentric<Graph> placer(g);
  placer.PlaceBetweenKNearest(NodeID(0), 1, filter);

  // BFS still walks through 1 and 2 to reach 3, but only 3 is collected.
  PositionComponent *p0 = positions->Find(0);
  REQUIRE(p0->pos[0] == Catch::Approx(30.0));
  REQUIRE(p0->pos[1] == Catch::Approx(0.0));
}
