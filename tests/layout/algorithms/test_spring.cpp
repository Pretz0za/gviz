#include "catch_amalgamated.hpp"
#include "layout/algorithms/spring.hpp"

#include "helpers/graph_helpers.hpp"

#include <array>
#include <cmath>

using test_helpers::MakeGraph;

namespace {
void SetPos(SpringLayoutAlgorithm<Graph> &alg, NodeID n, double x, double y) {
  std::array<double, 2> p{x, y};
  alg.SetPosition(n, p.data());
}
} // namespace

TEST_CASE("Construction registers the position/velocity/physics/mass/spring "
         "pools and a FixedNodesResource",
         "[layout][algorithms][spring]") {
  Graph g = MakeGraph(2, 2);
  g.AddUndirectedEdge(NodeID(0), NodeID(1));

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  SpringLayoutAlgorithm<Graph> alg(g, &algConfig);

  REQUIRE(g.NodeSpace().GetPool<PositionComponent>() != nullptr);
  REQUIRE(g.NodeSpace().GetPool<VelocityComponent>() != nullptr);
  REQUIRE(g.NodeSpace().GetPool<PhysicsComponent>() != nullptr);
  REQUIRE(g.NodeSpace().GetPool<MassComponent>() != nullptr);
  REQUIRE(g.EdgeSpace().GetPool<SpringComponent>() != nullptr);
  REQUIRE(g.HasResource<FixedNodesResource>());
}

TEST_CASE("Missing DimensionResource is reported instead of silently defaulting",
         "[layout][algorithms][spring]") {
  Graph g;
  g.AddNode();

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  REQUIRE_THROWS_AS((SpringLayoutAlgorithm<Graph>(g, &algConfig)),
                    MissingResourceException<DimensionResource>);
}

TEST_CASE("A stretched spring pulls its two free endpoints toward the rest length",
         "[layout][algorithms][spring]") {
  Graph g = MakeGraph(2, 2);
  g.AddUndirectedEdge(NodeID(0), NodeID(1));

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  algConfig.stiffness = 1.0;
  algConfig.restLength = 1.0;
  algConfig.damping = 0.9;
  algConfig.timeStep = 0.05;
  SpringLayoutAlgorithm<Graph> alg(g, &algConfig);

  SetPos(alg, NodeID(0), 0.0, 0.0);
  SetPos(alg, NodeID(1), 5.0, 0.0);

  for (int i = 0; i < 400; i++)
    alg.Tick();

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  double finalDist =
      std::abs(positions->Find(1)->pos[0] - positions->Find(0)->pos[0]);

  REQUIRE(finalDist < 5.0);
  REQUIRE(finalDist == Catch::Approx(1.0).margin(0.05));
}

TEST_CASE("A compressed spring pushes its two free endpoints apart toward the "
         "rest length",
         "[layout][algorithms][spring]") {
  Graph g = MakeGraph(2, 2);
  g.AddUndirectedEdge(NodeID(0), NodeID(1));

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  algConfig.stiffness = 1.0;
  algConfig.restLength = 3.0;
  algConfig.damping = 0.9;
  algConfig.timeStep = 0.05;
  SpringLayoutAlgorithm<Graph> alg(g, &algConfig);

  SetPos(alg, NodeID(0), 0.0, 0.0);
  SetPos(alg, NodeID(1), 0.5, 0.0);

  for (int i = 0; i < 400; i++)
    alg.Tick();

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  double finalDist =
      std::abs(positions->Find(1)->pos[0] - positions->Find(0)->pos[0]);

  REQUIRE(finalDist > 0.5);
  REQUIRE(finalDist == Catch::Approx(3.0).margin(0.05));
}

TEST_CASE("A fixed vertex never moves no matter the spring force on it, "
         "while its free neighbor still does",
         "[layout][algorithms][spring]") {
  Graph g = MakeGraph(2, 2);
  g.AddUndirectedEdge(NodeID(0), NodeID(1));

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  algConfig.stiffness = 1.0;
  algConfig.restLength = 0.0;
  algConfig.damping = 0.5;
  algConfig.timeStep = 0.05;
  SpringLayoutAlgorithm<Graph> alg(g, &algConfig);

  SetPos(alg, NodeID(0), 0.0, 0.0);
  SetPos(alg, NodeID(1), 5.0, 0.0);
  alg.Fix(NodeID(0));
  REQUIRE(alg.IsFixed(NodeID(0)));
  REQUIRE_FALSE(alg.IsFixed(NodeID(1)));

  for (int i = 0; i < 100; i++)
    alg.Tick();

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  REQUIRE(positions->Find(0)->pos[0] == Catch::Approx(0.0));
  REQUIRE(positions->Find(0)->pos[1] == Catch::Approx(0.0));
  REQUIRE(positions->Find(1)->pos[0] < 5.0);
}

TEST_CASE("Unfix lets a previously fixed vertex move again",
         "[layout][algorithms][spring]") {
  Graph g = MakeGraph(2, 2);
  g.AddUndirectedEdge(NodeID(0), NodeID(1));

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  algConfig.stiffness = 1.0;
  algConfig.restLength = 0.0;
  algConfig.damping = 0.5;
  algConfig.timeStep = 0.05;
  SpringLayoutAlgorithm<Graph> alg(g, &algConfig);

  SetPos(alg, NodeID(0), 0.0, 0.0);
  SetPos(alg, NodeID(1), 5.0, 0.0);
  alg.Fix(NodeID(0));

  for (int i = 0; i < 20; i++)
    alg.Tick();

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  REQUIRE(positions->Find(0)->pos[0] == Catch::Approx(0.0));

  alg.Unfix(NodeID(0));
  REQUIRE_FALSE(alg.IsFixed(NodeID(0)));

  for (int i = 0; i < 20; i++)
    alg.Tick();

  REQUIRE(positions->Find(0)->pos[0] != Catch::Approx(0.0));
}

TEST_CASE("SetPosition both teleports a vertex and clears its carried velocity",
         "[layout][algorithms][spring]") {
  Graph g = MakeGraph(2, 2);
  g.AddUndirectedEdge(NodeID(0), NodeID(1));

  SpringLayoutAlgorithm<Graph>::Configuration algConfig;
  algConfig.stiffness = 1.0;
  algConfig.restLength = 0.0;
  algConfig.damping = 0.0;
  algConfig.timeStep = 0.1;
  SpringLayoutAlgorithm<Graph> alg(g, &algConfig);

  alg.Fix(NodeID(0));
  SetPos(alg, NodeID(0), 0.0, 0.0);
  SetPos(alg, NodeID(1), 5.0, 0.0);

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();

  double prev = positions->Find(1)->pos[0];
  for (int i = 0; i < 20; i++)
    alg.Tick();
  double beforeResetStep = prev - positions->Find(1)->pos[0];
  prev = positions->Find(1)->pos[0];
  alg.Tick();
  double lastFreeStep = prev - positions->Find(1)->pos[0];

  double resetTarget = positions->Find(1)->pos[0];
  SetPos(alg, NodeID(1), resetTarget, 0.0);

  prev = positions->Find(1)->pos[0];
  alg.Tick();
  double afterResetStep = prev - positions->Find(1)->pos[0];

  REQUIRE(std::abs(beforeResetStep) > 0.0);
  REQUIRE(std::abs(afterResetStep) < std::abs(lastFreeStep) * 0.5);
}
