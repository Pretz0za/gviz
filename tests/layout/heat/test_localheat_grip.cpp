// LocalHeatGrip implements the local temperature rule from the GRIP paper
// (page 5): a node's temperature grows when its displacement keeps pointing
// the same way tick over tick (i.e. it's making steady progress) and shrinks
// when it starts oscillating (the new displacement points opposite the old
// one). Each test manipulates PhysicsComponent/LocalHeatComponent directly
// so a single Tick() call can be checked in isolation, rather than needing
// to replay several ticks to reach a particular state.

#include "catch_amalgamated.hpp"
#include "layout/heat/localheat_grip.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::WithLocalHeat;
using test_helpers::WithPhysics;

TEST_CASE("With no displacement history, heat is set to the fixed initial value",
         "[layout][heat][localheat_grip]") {
  Graph g = MakeGraph(1, 2);
  auto *physics = WithPhysics(g);
  auto *heat = WithLocalHeat(g);
  heat->Find(0)->heat = 99.0; // should be overwritten, not accumulated

  LocalHeatGrip<Graph> system(g);
  system.Tick(DenseNodeID(0));

  REQUIRE(heat->Find(0)->heat == Catch::Approx(10.0 / 6.0));
  REQUIRE(heat->Find(0)->oldCos == Catch::Approx(0.0));
}

TEST_CASE("A displacement pointing the same way as last tick raises the heat",
         "[layout][heat][localheat_grip]") {
  Graph g = MakeGraph(1, 2);
  auto *physics = WithPhysics(g);
  auto *heat = WithLocalHeat(g);

  PhysicsComponent *p = physics->Find(0);
  p->disp[0] = 1.0;
  p->disp[1] = 0.0;
  p->oldDisp[0] = 1.0;
  p->oldDisp[1] = 0.0;
  heat->Find(0)->heat = 2.0;
  heat->Find(0)->oldCos = 0.0; // not > 0, so the "steady streak" bonus doesn't apply yet

  LocalHeatGrip<Graph> system(g);
  system.Tick(DenseNodeID(0));

  // cos = 1, oldCos was 0 (not > 0) -> heat *= (1 + cos * 0.15) = 1.15
  REQUIRE(heat->Find(0)->heat == Catch::Approx(2.0 * 1.15));
  REQUIRE(heat->Find(0)->oldCos == Catch::Approx(1.0));
}

TEST_CASE("Two consecutive ticks pointing the same way compound the bonus",
         "[layout][heat][localheat_grip]") {
  Graph g = MakeGraph(1, 2);
  auto *physics = WithPhysics(g);
  auto *heat = WithLocalHeat(g);

  PhysicsComponent *p = physics->Find(0);
  p->disp[0] = 1.0;
  p->disp[1] = 0.0;
  p->oldDisp[0] = 1.0;
  p->oldDisp[1] = 0.0;
  heat->Find(0)->heat = 2.0;
  heat->Find(0)->oldCos = 0.5; // already positive from a prior tick

  LocalHeatGrip<Graph> system(g);
  system.Tick(DenseNodeID(0));

  // cos = 1, oldCos > 0 -> heat *= (1 + cos * 0.15 * 3.0) = 1.45
  REQUIRE(heat->Find(0)->heat == Catch::Approx(2.0 * 1.45));
}

TEST_CASE("A displacement reversing direction cools the node down",
         "[layout][heat][localheat_grip]") {
  Graph g = MakeGraph(1, 2);
  auto *physics = WithPhysics(g);
  auto *heat = WithLocalHeat(g);

  PhysicsComponent *p = physics->Find(0);
  p->disp[0] = -1.0;
  p->disp[1] = 0.0;
  p->oldDisp[0] = 1.0;
  p->oldDisp[1] = 0.0;
  heat->Find(0)->heat = 2.0;
  heat->Find(0)->oldCos = 1.0;

  LocalHeatGrip<Graph> system(g);
  system.Tick(DenseNodeID(0));

  // cos = -1 -> heat *= (1 + (-1) * 0.15) = 0.85
  REQUIRE(heat->Find(0)->heat == Catch::Approx(2.0 * 0.85));
  REQUIRE(heat->Find(0)->oldCos == Catch::Approx(-1.0));
}

TEST_CASE("A zero displacement leaves heat and oldCos unchanged",
         "[layout][heat][localheat_grip]") {
  Graph g = MakeGraph(1, 2);
  auto *physics = WithPhysics(g);
  auto *heat = WithLocalHeat(g);

  PhysicsComponent *p = physics->Find(0);
  p->disp[0] = 0.0;
  p->disp[1] = 0.0;
  p->oldDisp[0] = 1.0;
  p->oldDisp[1] = 0.0;
  heat->Find(0)->heat = 3.0;
  heat->Find(0)->oldCos = 0.7;

  LocalHeatGrip<Graph> system(g);
  system.Tick(DenseNodeID(0));

  REQUIRE(heat->Find(0)->heat == Catch::Approx(3.0));
  REQUIRE(heat->Find(0)->oldCos == Catch::Approx(0.7));
}
