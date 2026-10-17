// Unit tests for the LinLog force model (ported from grapher-old's
// ForceModel::LinLog).

#include "catch_amalgamated.hpp"
#include "layout/physics/force_model/linlog.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::SetPosition;
using test_helpers::WithPhysics;
using test_helpers::WithPositions;
using test_helpers::WithRadius;

namespace {
void SetRadius(DenseComponentPool<RadiusComponent> *pool, uint32_t id, double r) {
  pool->Find(id)->radius = r;
}
} // namespace

TEST_CASE("LinLog AttractiveTick pulls v toward u by log(1 + gap)",
         "[layout][physics][linlog]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  LinLog<Graph>::Configuration config;
  LinLog<Graph> model(g, &config);
  model.AttractiveTick(DenseNodeID(0), DenseNodeID(1));

  // gap = 10, magnitude = log(1 + 10), direction toward u is (1, 0).
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(std::log1p(10.0)));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("LinLog RepulsiveTick(v, u) pushes v away from u, scaled by mass product / gap",
         "[layout][physics][linlog]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  LinLog<Graph>::Configuration config;
  LinLog<Graph> model(g, &config);
  model.RepulsiveTick(DenseNodeID(0), DenseNodeID(1));

  // Neither node has any edges, so both masses are (1 + 0) = 1.
  // gap = 10, mag = (1*1)/10 = 0.1, direction away from u is (-1, 0).
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.1));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("LinLog RepulsiveTick(v, u) scales with degree-derived vertex mass",
         "[layout][physics][linlog]") {
  Graph g = MakeGraph(3, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetPosition(positions, 2, {-10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);
  SetRadius(radii, 2, 0.0);

  // Node 1 picks up degree 2 (mass 3) from two edges with node 2, leaving
  // node 0 untouched at degree 0 (mass 1).
  g.AddEdge(NodeID(1), NodeID(2));
  g.AddEdge(NodeID(2), NodeID(1));

  LinLog<Graph>::Configuration config;
  LinLog<Graph> model(g, &config);
  model.RepulsiveTick(DenseNodeID(0), DenseNodeID(1));

  // gap = 10, mag = (1 * 3) / 10 = 0.3, direction away from u is (-1, 0).
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.3));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("LinLog RepulsiveTick(v, pointer, mass) matches the two-node form "
         "when passed u's own position and mass",
         "[layout][physics][linlog]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});

  LinLog<Graph>::Configuration config;
  LinLog<Graph> model(g, &config);
  double uPos[2] = {10.0, 0.0};
  model.RepulsiveTick(DenseNodeID(0), uPos, 1.0);

  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.1));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("LinLog ticks stay finite when nodes exactly coincide",
         "[layout][physics][linlog]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  WithRadius(g);
  SetPosition(positions, 0, {5.0, 5.0});
  SetPosition(positions, 1, {5.0, 5.0});

  LinLog<Graph>::Configuration config;
  LinLog<Graph> model(g, &config);

  model.AttractiveTick(DenseNodeID(0), DenseNodeID(1));
  double *attractiveDisp = physics->Find(0)->disp;
  REQUIRE(std::isfinite(attractiveDisp[0]));
  REQUIRE(std::isfinite(attractiveDisp[1]));

  model.RepulsiveTick(DenseNodeID(0), DenseNodeID(1));
  double *disp = physics->Find(0)->disp;
  REQUIRE(std::isfinite(disp[0]));
  REQUIRE(std::isfinite(disp[1]));

  double uPos[2] = {5.0, 5.0};
  model.RepulsiveTick(DenseNodeID(0), uPos, 1.0);
  REQUIRE(std::isfinite(disp[0]));
  REQUIRE(std::isfinite(disp[1]));
}
