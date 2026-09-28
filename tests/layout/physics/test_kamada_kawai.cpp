// GRIPKamadaKawai::Tick nudges v toward/away from u based on how its actual
// boundary gap compares to an "ideal" distance derived from graphDist * 100.

#include "catch_amalgamated.hpp"
#include "layout/physics/force_model/kamada_kawai.hpp"

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

TEST_CASE("Tick pulls v toward u when the gap is smaller than the ideal distance",
         "[layout][physics][kamada_kawai]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  GRIPKamadaKawai<Graph> kk(g);
  kk.Tick(DenseNodeID(0), DenseNodeID(1), /*graphDist=*/2);

  // gap = 10, factor = 10 / (2 * 100) - 1 = -0.95, direction (1, 0) * gap
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-9.5));
  REQUIRE(disp[1] == Catch::Approx(0.0));
}

TEST_CASE("A larger graphDist lowers the ideal distance, making the pull stronger",
         "[layout][physics][kamada_kawai]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);

  GRIPKamadaKawai<Graph> kk(g);
  kk.Tick(DenseNodeID(0), DenseNodeID(1), /*graphDist=*/1);

  // factor = 10 / (1 * 100) - 1 = -0.9
  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(-9.0));
}

TEST_CASE("Tick accumulates into any pre-existing displacement",
         "[layout][physics][kamada_kawai]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  auto *physics = WithPhysics(g);
  auto *radii = WithRadius(g);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);
  physics->Find(0)->disp[0] = 1.0;

  GRIPKamadaKawai<Graph> kk(g);
  kk.Tick(DenseNodeID(0), DenseNodeID(1), 2);

  double *disp = physics->Find(0)->disp;
  REQUIRE(disp[0] == Catch::Approx(1.0 - 9.5));
}
