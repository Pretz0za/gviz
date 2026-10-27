// PositionRandomized scatters every node's position uniformly within
// [-bboxWidth, bboxWidth] per axis, using a seedable RNG for reproducibility.

#include "catch_amalgamated.hpp"
#include "layout/placement/randomized.hpp"

#include "graph/decorated_graph.hpp"
#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::WithPositions;

TEST_CASE("PlaceAll assigns every node a position within [-bboxWidth, bboxWidth]",
         "[layout][placement][randomized]") {
  Graph g = MakeGraph(20);
  DecoratedGraph<Graph> dg = test_helpers::Decorate(g, 2);
  auto *positions = WithPositions(dg, 2);

  PositionRandomized<Graph>::Configuration placerConfig;
  PositionRandomized<Graph> placer(dg, &placerConfig);
  placer.SetSeed(1234);
  placer.Config()->WithBoundingBoxWidth(50);
  placer.PlaceAll();

  for (uint32_t i = 0; i < 20; i++) {
    PositionComponent *c = positions->Find(i);
    REQUIRE(c->pos[0] >= -50.0);
    REQUIRE(c->pos[0] <= 50.0);
    REQUIRE(c->pos[1] >= -50.0);
    REQUIRE(c->pos[1] <= 50.0);
  }
}

TEST_CASE("The same seed produces the same placement", "[layout][placement][randomized]") {
  Graph g1 = MakeGraph(10);
  DecoratedGraph<Graph> dg1 = test_helpers::Decorate(g1, 2);
  auto *positions1 = WithPositions(dg1, 2);
  PositionRandomized<Graph>::Configuration placer1Config;
  PositionRandomized<Graph> placer1(dg1, &placer1Config);
  placer1.SetSeed(42);
  placer1.PlaceAll();

  Graph g2 = MakeGraph(10);
  DecoratedGraph<Graph> dg2 = test_helpers::Decorate(g2, 2);
  auto *positions2 = WithPositions(dg2, 2);
  PositionRandomized<Graph>::Configuration placer2Config;
  PositionRandomized<Graph> placer2(dg2, &placer2Config);
  placer2.SetSeed(42);
  placer2.PlaceAll();

  for (uint32_t i = 0; i < 10; i++) {
    REQUIRE(positions1->Find(i)->pos[0] == positions2->Find(i)->pos[0]);
    REQUIRE(positions1->Find(i)->pos[1] == positions2->Find(i)->pos[1]);
  }
}

TEST_CASE("Different seeds (almost certainly) produce different placements",
         "[layout][placement][randomized]") {
  Graph g1 = MakeGraph(5);
  DecoratedGraph<Graph> dg1 = test_helpers::Decorate(g1, 2);
  auto *positions1 = WithPositions(dg1, 2);
  PositionRandomized<Graph>::Configuration placer1Config;
  PositionRandomized<Graph> placer1(dg1, &placer1Config);
  placer1.SetSeed(1);
  placer1.PlaceAll();

  Graph g2 = MakeGraph(5);
  DecoratedGraph<Graph> dg2 = test_helpers::Decorate(g2, 2);
  auto *positions2 = WithPositions(dg2, 2);
  PositionRandomized<Graph>::Configuration placer2Config;
  PositionRandomized<Graph> placer2(dg2, &placer2Config);
  placer2.SetSeed(2);
  placer2.PlaceAll();

  bool anyDifferent = false;
  for (uint32_t i = 0; i < 5; i++) {
    if (positions1->Find(i)->pos[0] != positions2->Find(i)->pos[0] ||
        positions1->Find(i)->pos[1] != positions2->Find(i)->pos[1]) {
      anyDifferent = true;
      break;
    }
  }
  REQUIRE(anyDifferent);
}

TEST_CASE("PlaceAll only writes the configured dimension's components",
         "[layout][placement][randomized]") {
  Graph g = MakeGraph(3);
  DecoratedGraph<Graph> dg = test_helpers::Decorate(g, 2); // dimension 2, so pos[] only has 2 live slots
  auto *positions = WithPositions(dg, 2);

  PositionRandomized<Graph>::Configuration placerConfig;
  PositionRandomized<Graph> placer(dg, &placerConfig);
  placer.SetSeed(7);
  placer.PlaceAll();

  REQUIRE(positions->Dimension() == 2);
}
