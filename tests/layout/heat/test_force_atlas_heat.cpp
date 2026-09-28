// ForceAtlasHeat implements ForceAtlas2's global adaptive-speed control
// (Jacomy et al., 2014): each node's swinging/traction contribute to one
// graph-wide speed, and each node is damped by its own swinging via
// SpeedFactor.

#include "catch_amalgamated.hpp"
#include "layout/heat/force_atlas.hpp"

#include "helpers/graph_helpers.hpp"

#include <cmath>

namespace {
DenseComponentPool<ForceAtlasHeatComponent> *
WithForceAtlasHeat(Graph &g) {
  return g.NodeSpace().SetPool<ForceAtlasHeatComponent>();
}
} // namespace

using test_helpers::MakeGraph;

TEST_CASE("Observe measures swinging as the change in force since last tick",
         "[layout][heat][force_atlas]") {
  Graph g = MakeGraph(1, 2);
  auto *heatPool = WithForceAtlasHeat(g);

  ForceAtlasHeat<Graph> heat(g);
  heat.BeginTick();

  double force[2] = {1.0, 0.0};
  heat.Observe(DenseNodeID(0), force);

  // oldForce starts at (0, 0), so diff = force - oldForce = (1, 0):
  // swinging = |diff| = 1.0.
  REQUIRE(heatPool->Find(0)->swinging == Catch::Approx(1.0));
}

TEST_CASE("Observe with a repeated identical force reports zero swinging",
         "[layout][heat][force_atlas]") {
  Graph g = MakeGraph(1, 2);
  WithForceAtlasHeat(g);

  ForceAtlasHeat<Graph> heat(g);
  double force[2] = {2.0, 3.0};

  heat.BeginTick();
  heat.Observe(DenseNodeID(0), force);
  heat.BeginTick();
  heat.Observe(DenseNodeID(0), force); // same force again -> no change

  // swinging is now 0, so SpeedFactor == globalSpeed / (1 + 0) == globalSpeed,
  // which is still its untouched initial value of 1.0 (UpdateGlobalSpeed was
  // never called in this test).
  REQUIRE(heat.SpeedFactor(DenseNodeID(0)) == Catch::Approx(1.0));
}

TEST_CASE("SpeedFactor decreases as a node's swinging increases", "[layout][heat][force_atlas]") {
  Graph g = MakeGraph(2, 2);
  WithForceAtlasHeat(g);

  ForceAtlasHeat<Graph> heat(g);

  heat.BeginTick();
  double smallForce[2] = {0.1, 0.0};
  double bigForce[2] = {10.0, 0.0};
  heat.Observe(DenseNodeID(0), smallForce);
  heat.Observe(DenseNodeID(1), bigForce);

  double speed0 = heat.SpeedFactor(DenseNodeID(0));
  double speed1 = heat.SpeedFactor(DenseNodeID(1));

  REQUIRE(speed0 > speed1);
}

TEST_CASE("UpdateGlobalSpeed leaves the global speed at its initial value "
         "when no traction has been observed",
         "[layout][heat][force_atlas]") {
  Graph g = MakeGraph(1, 2);
  WithForceAtlasHeat(g);

  ForceAtlasHeat<Graph> heat(g);
  heat.BeginTick();
  // No Observe() call: totalTraction stays 0, below the epsilon guard.
  heat.UpdateGlobalSpeed(1);

  // With swinging == 0 (never observed), SpeedFactor == globalSpeed exactly.
  REQUIRE(heat.SpeedFactor(DenseNodeID(0)) == Catch::Approx(1.0));
}

TEST_CASE("UpdateGlobalSpeed drops global speed toward the jitter-tolerance-"
         "derived target on its first update",
         "[layout][heat][force_atlas]") {
  Graph g = MakeGraph(1, 2);
  WithForceAtlasHeat(g);

  ForceAtlasHeat<Graph> heat(g);

  double force[2] = {1.0, 0.0};
  heat.BeginTick();
  heat.Observe(DenseNodeID(0), force); // oldForce (0,0) -> swinging=1, traction=0.5
  heat.UpdateGlobalSpeed(1);

  // n=1: estimatedOptimalJT = 0.05, minJT = sqrt(0.05), and
  // estimatedOptimalJT*traction/n^2 = 0.025 < minJT, so jt = sqrt(0.05).
  // swinging/traction == 2.0 exactly (not > 2.0), so the "erratic" branch
  // does not fire yet; targetSpeed = jt * speedEfficiency(1) * traction /
  // swinging = jt * 0.5. Since targetSpeed - globalSpeed(1.0) is negative
  // and smaller in magnitude than kSpeedMaxRise * globalSpeed (0.5), the
  // update moves globalSpeed all the way to targetSpeed.
  double expected = std::sqrt(0.05) * 0.5;

  // Observe an identical force again so swinging drops to 0 and
  // SpeedFactor(id) == globalSpeed exactly (division by 1 + sqrt(0)).
  heat.BeginTick();
  heat.Observe(DenseNodeID(0), force);

  REQUIRE(heat.SpeedFactor(DenseNodeID(0)) == Catch::Approx(expected).margin(1e-9));
}
