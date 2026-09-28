// Integration tests for GRIPLayoutAlgorithm, which drives MisFiltrationSystem,
// PositionBarrycentric, and GRIPPhysicsSystem through GRIP's coarsen-then-
// refine pipeline: filter the graph into nested layers, place the coarsest
// layer with a regular simplex, then walk back down placing each finer
// layer between its already-placed neighbors and running physics on it.
//
// Unlike ForceDirectedLayoutAlgorithm (see test_force_directed.cpp), GRIP's
// path never touches the QuadTree/Barnes-Hut code, so it's safe to run
// end-to-end here.

#include "catch_amalgamated.hpp"
#include "layout/algorithms/grip.hpp"

#include "helpers/graph_helpers.hpp"

#include <cmath>

using test_helpers::MakeGraph;

namespace {
Graph MakePathGraph(uint32_t length) {
  Graph g = MakeGraph(length, 2);
  for (uint32_t i = 0; i + 1 < length; i++)
    g.AddUndirectedEdge(NodeID(i), NodeID(i + 1));
  return g;
}
} // namespace

TEST_CASE("RunFiltration builds the same nested layers MisFiltrationSystem "
         "produces standalone",
         "[layout][algorithms][grip]") {
  Graph g = MakePathGraph(6);
  GRIPLayoutAlgorithm<Graph> alg(g);
  alg.RunFiltration();

  auto *filtration = g.GetResource<NestedFiltrationResult>();
  REQUIRE(filtration->m_layerCount == 2);
  REQUIRE(filtration->m_borders[0] == 6);
  REQUIRE(filtration->m_borders[1] == 3);
}

TEST_CASE("Tick before RunFiltration throws UinitializedResourceException",
         "[layout][algorithms][grip]") {
  Graph g = MakePathGraph(4);
  GRIPLayoutAlgorithm<Graph> alg(g);

  REQUIRE_THROWS_AS(alg.Tick(), UinitializedResourceException<NestedFiltrationResult>);
}

TEST_CASE("Walking TransitionState down to 0 places every node with finite "
         "coordinates and marks the whole graph visible",
         "[layout][algorithms][grip]") {
  Graph g = MakePathGraph(6);
  GRIPLayoutAlgorithm<Graph> alg(g);
  alg.RunFiltration();

  uint32_t layer = 0;
  int guard = 0;
  do {
    layer = alg.TransitionState();
  } while (layer != 0 && ++guard < 100);

  REQUIRE(guard < 100); // didn't loop forever

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  for (uint32_t i = 0; i < g.Size(); i++) {
    PositionComponent *p = positions->Find(i);
    REQUIRE(std::isfinite(p->pos[0]));
    REQUIRE(std::isfinite(p->pos[1]));
  }

  auto *visible = g.GetResource<VisibleNodesResource>();
  for (uint32_t i = 0; i < g.Size(); i++)
    REQUIRE(visible->Test(DenseNodeID(i)));
}

TEST_CASE("Tick after the full transition sequence runs without error and "
         "keeps positions finite",
         "[layout][algorithms][grip]") {
  Graph g = MakePathGraph(6);
  GRIPLayoutAlgorithm<Graph> alg(g);
  alg.RunFiltration();

  uint32_t layer = 0;
  int guard = 0;
  do {
    layer = alg.TransitionState();
  } while (layer != 0 && ++guard < 100);

  REQUIRE_NOTHROW(alg.Tick());

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  for (uint32_t i = 0; i < g.Size(); i++) {
    PositionComponent *p = positions->Find(i);
    REQUIRE(std::isfinite(p->pos[0]));
    REQUIRE(std::isfinite(p->pos[1]));
  }
}
