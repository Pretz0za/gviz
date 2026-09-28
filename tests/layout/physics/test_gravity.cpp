// GravityForceSystem pulls every node toward the origin, scaled by
// (degree + 1) and an overall magnitude, and skips nodes already at the
// origin (division by zero distance is guarded against).

#include "catch_amalgamated.hpp"
#include "layout/physics/gravity.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::SetPosition;
using test_helpers::WithPhysics;
using test_helpers::WithPositions;

TEST_CASE("An isolated node is pulled toward the origin by -magnitude/dist * pos",
         "[layout][physics][gravity]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  SetPosition(positions, 0, {3.0, 4.0});

  GravityForceSystem<Graph> gravity(g);
  gravity.Tick();

  // dist = 5, degree = 0 -> factor = -0.2 * 1 / 5 = -0.04
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.12));
  REQUIRE(disp[1] == Catch::Approx(-0.16));
}

TEST_CASE("Higher degree nodes are pulled proportionally harder",
         "[layout][physics][gravity]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  g.AddEdge(NodeID(0), NodeID(1)); // gives node 0 degree 1

  SetPosition(positions, 0, {6.0, 8.0});
  SetPosition(positions, 1, {0.0, 0.0});

  GravityForceSystem<Graph> gravity(g);
  gravity.Tick();

  // dist = 10, degree = 1 -> factor = -0.2 * 2 / 10 = -0.04
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.24));
  REQUIRE(disp[1] == Catch::Approx(-0.32));
}

TEST_CASE("A node already at the origin is left untouched", "[layout][physics][gravity]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  SetPosition(positions, 0, {0.0, 0.0});

  GravityForceSystem<Graph> gravity(g);
  gravity.Tick();

  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(0.0));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("SetMagnitude scales the pull linearly", "[layout][physics][gravity]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  SetPosition(positions, 0, {3.0, 4.0});

  GravityForceSystem<Graph> gravity(g);
  gravity.SetMagnitude(1.0);
  gravity.Tick();

  // dist = 5, degree = 0 -> factor = -1.0 * 1 / 5 = -0.2
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.6));
  REQUIRE(disp[1] == Catch::Approx(-0.8));
}

TEST_CASE("Tick accumulates into existing displacement rather than overwriting it",
         "[layout][physics][gravity]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  SetPosition(positions, 0, {3.0, 4.0});
  physics->Find(0)->disp[0] = 100.0;
  physics->Find(0)->disp[1] = 100.0;

  GravityForceSystem<Graph> gravity(g);
  gravity.Tick();

  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(100.0 - 0.12));
  REQUIRE(disp[1] == Catch::Approx(100.0 - 0.16));
}
