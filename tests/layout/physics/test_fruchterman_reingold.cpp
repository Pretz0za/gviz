// Unit tests for the two Fruchterman-Reingold force models.
//
// NOTE: GRIPFruchtermanReingold::RepulsiveTick(v, const double*, double) --
// the 3-argument overload used for Barnes-Hut quadtree approximation -- is
// NOT exercised here. Its body calls `m_distanceCalc.ToPoint(...)`, but
// DistanceCalculationSystem has no member named ToPoint (only
// DistanceToPoint). That's a compile error the moment this specific
// overload is instantiated, so tests avoid calling it until fixed.
// VanillaFruchtermanReingold's 3-argument overload has no such bug and is
// tested normally.

#include "catch_amalgamated.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"

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

TEST_CASE("Vanilla AttractiveTick pulls v toward u by (gap / edgeLength)",
         "[layout][physics][fruchterman_reingold]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  VanillaFruchtermanReingold<Graph> fr(g);
  fr.AttractiveTick(DenseNodeID(0), DenseNodeID(1));

  // gap = 10, edgeLength = 10 -> factor 1, direction (1, 0)
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(10.0));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("Vanilla RepulsiveTick(v, u) pushes v away from u",
         "[layout][physics][fruchterman_reingold]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  VanillaFruchtermanReingold<Graph> fr(g);
  fr.RepulsiveTick(DenseNodeID(0), DenseNodeID(1));

  // gap = 10, edgeLength^2 / gap^2 = 1, direction away from u is (-1, 0)
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-10.0));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("Vanilla RepulsiveTick(v, pointer, mass) matches the two-node form "
         "when passed u's own position",
         "[layout][physics][fruchterman_reingold]") {
  // Regression test: fruchterman_reingold.tpp's comment says this overload
  // computes "v - u (not u - v)" to push v away from u, matching the
  // 2-argument RepulsiveTick(v, u) above, but the code actually computes
  // `Subtract(uPos, vPos.pos, out, dim)` (u - v), the opposite direction.
  // The expected value below is the direction the comment -- and the
  // 2-argument overload -- promise, i.e. what "correct" means here.
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});

  VanillaFruchtermanReingold<Graph> fr(g);
  double uPos[2] = {10.0, 0.0};
  fr.RepulsiveTick(DenseNodeID(0), uPos, 1.0);

  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-10.0));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("GRIP AttractiveTick scales by (gap^2 / 100)", "[layout][physics][fruchterman_reingold]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {20.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  GRIPFruchtermanReingold<Graph> fr(g);
  fr.AttractiveTick(DenseNodeID(0), DenseNodeID(1));

  // gap = 20, factor = 20^2 / 100 = 4, direction (1, 0) * gap = (20, 0)
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(80.0));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("GRIP RepulsiveTick(v, u) scales by (0.05 * 100 / gap^2)",
         "[layout][physics][fruchterman_reingold]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {20.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  GRIPFruchtermanReingold<Graph> fr(g);
  fr.RepulsiveTick(DenseNodeID(0), DenseNodeID(1));

  // gap = 20, factor = 0.05 * 100 / 400 = 0.0125, direction away from u * gap
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-0.25));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("GRIP AttractiveTick is a no-op when the two nodes coincide",
         "[layout][physics][fruchterman_reingold]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  WithRadius(g);
  SetPosition(positions, 0, {5.0, 5.0});
  SetPosition(positions, 1, {5.0, 5.0});

  GRIPFruchtermanReingold<Graph> fr(g);
  fr.AttractiveTick(DenseNodeID(0), DenseNodeID(1));

  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(0.0));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}
