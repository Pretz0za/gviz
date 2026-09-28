// GRIPPhysicsSystem ties together k-nearest lookup, GRIP's
// Fruchterman-Reingold/Kamada-Kawai force models, and LocalHeatGrip's local
// temperature control into one per-layer physics tick.
//
// These tests wire up a GRIPPhysicsSystem directly (bypassing
// GRIPLayoutAlgorithm) by hand-filling the NestedFiltrationResult and
// VisibleNodesResource resources it depends on, so the forces and layer
// bookkeeping can be checked independently of filtration/placement.

#include "catch_amalgamated.hpp"
#include "layout/physics/grip.hpp"

#include "helpers/graph_helpers.hpp"

#include <cmath>

using test_helpers::GripFixture;
using test_helpers::SetPosition;
using test_helpers::WithRadius;

namespace {
void SetRadius(DenseComponentPool<RadiusComponent> *pool, uint32_t id, double r) {
  pool->Find(id)->radius = r;
}
} // namespace

TEST_CASE("RefreshKNearest populates each node's k-nearest list via BFS",
         "[layout][physics][grip]") {
  // 0 -> 1 -> 2
  GripFixture fx(3, 2);
  fx.graph.AddEdge(NodeID(0), NodeID(1));
  fx.graph.AddEdge(NodeID(1), NodeID(2));
  fx.visible->SetAll();
  fx.filtration->m_borders = {3};
  fx.filtration->m_layerCount = 1;
  fx.filtration->m_filtration = {DenseNodeID(0), DenseNodeID(1), DenseNodeID(2)};

  GRIPPhysicsSystem<Graph> physics(fx.graph);
  physics.RefreshKNearest(0);

  auto &knn = fx.knearest->Data();
  REQUIRE(knn[0].size == 2);
  REQUIRE(knn[0].nearest[0].node == NodeID(1));
  REQUIRE(knn[0].nearest[0].depth == 1);
  REQUIRE(knn[0].nearest[1].node == NodeID(2));
  REQUIRE(knn[0].nearest[1].depth == 2);

  REQUIRE(knn[1].size == 1);
  REQUIRE(knn[1].nearest[0].node == NodeID(2));

  REQUIRE(knn[2].size == 0);
}

TEST_CASE("Tick at layer 0 applies FR attractive+repulsive forces, then "
         "normalizes displacement to the node's local heat",
         "[layout][physics][grip]") {
  GripFixture fx(2, 2);
  fx.graph.AddEdge(NodeID(0), NodeID(1));
  auto *radii = WithRadius(fx.graph);
  SetRadius(radii, 0, 0.0);
  SetRadius(radii, 1, 0.0);
  SetPosition(fx.positions, 0, {0.0, 0.0});
  SetPosition(fx.positions, 1, {20.0, 0.0});

  fx.visible->SetAll();
  fx.filtration->m_borders = {2};
  fx.filtration->m_layerCount = 1;
  fx.filtration->m_filtration = {DenseNodeID(0), DenseNodeID(1)};

  GRIPPhysicsSystem<Graph> physics(fx.graph);
  physics.RefreshKNearest(0);
  physics.Tick();

  // Node 0 has both an attractive pull toward node 1 and a repulsive push
  // from it; whatever the net force direction, LocalHeatGrip caps a node's
  // very first displacement to a magnitude of 10/6 (see localheat_grip.tpp
  // -- no displacement history yet means heat = 10/6 unconditionally).
  auto &positions = fx.positions->Data();
  double dx = positions[0].pos[0] - 0.0;
  double dy = positions[0].pos[1] - 0.0;
  double moved = std::sqrt(dx * dx + dy * dy);
  REQUIRE(moved == Catch::Approx(10.0 / 6.0).margin(1e-9));
  REQUIRE(positions[0].pos[0] > 0.0); // net force pulled it toward node 1

  // Node 1 has no outgoing edges and no k-nearest neighbors in this
  // fixture, so it experiences no force and shouldn't move at all.
  REQUIRE(positions[1].pos[0] == Catch::Approx(20.0));
  REQUIRE(positions[1].pos[1] == Catch::Approx(0.0));
}

TEST_CASE("Tick is a no-op before RefreshKNearest establishes a current layer",
         "[layout][physics][grip]") {
  GripFixture fx(2, 2);
  WithRadius(fx.graph);
  SetPosition(fx.positions, 0, {1.0, 1.0});
  SetPosition(fx.positions, 1, {2.0, 2.0});
  fx.visible->SetAll();
  fx.filtration->m_borders = {2};
  fx.filtration->m_layerCount = 1;
  fx.filtration->m_filtration = {DenseNodeID(0), DenseNodeID(1)};

  GRIPPhysicsSystem<Graph> physics(fx.graph);
  physics.Tick(); // no RefreshKNearest call yet

  auto &positions = fx.positions->Data();
  REQUIRE(positions[0].pos[0] == Catch::Approx(1.0));
  REQUIRE(positions[1].pos[0] == Catch::Approx(2.0));
}
