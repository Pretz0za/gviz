// ForceDirectedLayoutAlgorithm wires up a Barnes-Hut quadtree, a pluggable
// ForceModel, gravity, and ForceAtlas2-style adaptive speed control.
//
// NOTE: Tick() is NOT called anywhere in this file. Its very first line
// dereferences `m_quadtree->root` (a `unique_ptr<QuadTree<...>>` inside the
// QuadTreeResource the constructor creates), but nothing in this codebase
// ever assigns an actual QuadTree into that `root` -- it stays default-
// constructed (null) for the object's whole lifetime. Calling Tick()
// dereferences that null pointer, which segfaults the whole test process,
// not just one test case. This matches the project's own history: the
// commit that wired Barnes-Hut into force_directed.tpp is titled "doesnt
// work though :)". Once QuadTreeResource::root is actually populated (e.g.
// built from SpacialIndex::Helpers::GetBoundingBox once per Tick), a
// Tick()-level test can be added here.

#include "catch_amalgamated.hpp"
#include "layout/algorithms/force_directed.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;

TEST_CASE("Construction registers position/physics/heat pools and a QuadTreeResource",
         "[layout][algorithms][force_directed]") {
  Graph g = MakeGraph(5, 2);

  ForceDirectedLayoutAlgorithm<Graph, VanillaFruchtermanReingold<Graph>> alg(g);

  REQUIRE(g.NodeSpace().GetPool<PositionComponent>() != nullptr);
  REQUIRE(g.NodeSpace().GetPool<PhysicsComponent>() != nullptr);
  REQUIRE(g.NodeSpace().GetPool<ForceAtlasHeatComponent>() != nullptr);
  REQUIRE(g.HasResource<QuadTreeResource>());
}

TEST_CASE("Construction randomizes every node's initial position within the "
         "default bounding box",
         "[layout][algorithms][force_directed]") {
  Graph g = MakeGraph(10, 2);
  ForceDirectedLayoutAlgorithm<Graph, VanillaFruchtermanReingold<Graph>> alg(g);

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  bool anyNonZero = false;
  for (uint32_t i = 0; i < 10; i++) {
    PositionComponent *p = positions->Find(i);
    if (p->pos[0] != 0.0 || p->pos[1] != 0.0)
      anyNonZero = true;
    // PositionRandomized's default bounding box is [-1000, 1000].
    REQUIRE(p->pos[0] >= -1000.0);
    REQUIRE(p->pos[0] <= 1000.0);
  }
  REQUIRE(anyNonZero);
}

TEST_CASE("Missing DimensionResource is reported instead of silently defaulting",
         "[layout][algorithms][force_directed]") {
  Graph g; // no DimensionResource set
  g.AddNode();

  REQUIRE_THROWS_AS(
      (ForceDirectedLayoutAlgorithm<Graph, VanillaFruchtermanReingold<Graph>>(g)),
      MissingResourceException<DimensionResource>);
}
