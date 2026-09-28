// DistanceCalculationSystem computes gaps between node *boundaries* (i.e.
// center-to-center distance minus both radii), not raw center distances, so
// two overlapping circles report a negative "distance".

#include "catch_amalgamated.hpp"
#include "layout/helpers/distance.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::SetPosition;
using test_helpers::WithPositions;
using test_helpers::WithRadius;

namespace {
void SetRadius(DenseComponentPool<RadiusComponent> *pool, uint32_t id,
              double radius) {
  pool->Find(id)->radius = radius;
}
} // namespace

TEST_CASE("BetweenNodes subtracts both radii from the center-to-center distance",
         "[layout][helpers][distance]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *radii = WithRadius(g);

  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 3.0);
  SetRadius(radii, 1, 3.0);

  DistanceCalculationSystem<Graph> dist(g);
  REQUIRE(dist.BetweenNodes(DenseNodeID(0), DenseNodeID(1)) == Catch::Approx(4.0));
}

TEST_CASE("BetweenNodes is negative when circles overlap", "[layout][helpers][distance]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *radii = WithRadius(g);

  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {5.0, 0.0});
  SetRadius(radii, 0, 4.0);
  SetRadius(radii, 1, 4.0);

  DistanceCalculationSystem<Graph> dist(g);
  REQUIRE(dist.BetweenNodes(DenseNodeID(0), DenseNodeID(1)) == Catch::Approx(-3.0));
}

TEST_CASE("VecBetweenNodes points from v to u, scaled to the boundary gap",
         "[layout][helpers][distance]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *radii = WithRadius(g);

  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 3.0);
  SetRadius(radii, 1, 3.0);

  DistanceCalculationSystem<Graph> dist(g);
  double out[2];
  dist.VecBetweenNodes(DenseNodeID(0), DenseNodeID(1), out);

  // raw distance 10, gap 4, direction (1, 0) -> scaled vector (4, 0)
  REQUIRE(out[0] == Catch::Approx(4.0));
  REQUIRE(out[1] == Catch::Approx(0.0));
}

TEST_CASE("VecBetweenNodes returns zero when the two nodes coincide",
         "[layout][helpers][distance]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  WithRadius(g);

  SetPosition(positions, 0, {5.0, 5.0});
  SetPosition(positions, 1, {5.0, 5.0});

  DistanceCalculationSystem<Graph> dist(g);
  double out[2] = {1.0, 1.0};
  dist.VecBetweenNodes(DenseNodeID(0), DenseNodeID(1), out);

  REQUIRE(out[0] == Catch::Approx(0.0));
  REQUIRE(out[1] == Catch::Approx(0.0));
}

TEST_CASE("VecToPoint returns p minus the node's position", "[layout][helpers][distance]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  WithRadius(g);
  SetPosition(positions, 0, {1.0, 1.0});

  DistanceCalculationSystem<Graph> dist(g);
  double p[2] = {4.0, 5.0};
  double out[2];
  dist.VecToPoint(DenseNodeID(0), p, out);

  REQUIRE(out[0] == Catch::Approx(3.0));
  REQUIRE(out[1] == Catch::Approx(4.0));
}

TEST_CASE("DistanceToPoint computes the plain Euclidean distance",
         "[layout][helpers][distance]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});

  DistanceCalculationSystem<Graph> dist(g);
  double p[2] = {3.0, 4.0};
  REQUIRE(dist.DistanceToPoint(DenseNodeID(0), p) == Catch::Approx(5.0));
}
