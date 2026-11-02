#include "catch_amalgamated.hpp"
#include "layout/algorithms/reingold_tilford.hpp"
#include <chrono>
#include <cmath>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeDirectedGraph;

namespace {

double X(ReingoldTilfordLayoutAlgorithm<Graph> &alg, NodeID n) {
  DenseNodeID d = alg.Decorated().graph->MapToDense(n);
  return alg.Decorated()
      .nodeSpace.template GetPool<PositionComponent>()
      ->Find(d.Raw())
      ->pos[0];
}

double Y(ReingoldTilfordLayoutAlgorithm<Graph> &alg, NodeID n) {
  DenseNodeID d = alg.Decorated().graph->MapToDense(n);
  return alg.Decorated()
      .nodeSpace.template GetPool<PositionComponent>()
      ->Find(d.Raw())
      ->pos[1];
}

} // namespace

TEST_CASE("Single node places at origin", "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(1);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, nodes[0], &cfg);
  alg.PlaceAll();

  REQUIRE(alg.Done());
  REQUIRE(X(alg, nodes[0]) == Catch::Approx(0.0));
  REQUIRE(Y(alg, nodes[0]) == Catch::Approx(0.0));
}

TEST_CASE("Root with two leaves is symmetric", "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(3);
  auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(n[0], n[1]);
  g.AddEdge(n[0], n[2]);

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, n[0], &cfg);
  alg.PlaceAll();

  REQUIRE(Y(alg, n[0]) == Catch::Approx(0.0));
  REQUIRE(Y(alg, n[1]) == Catch::Approx(cfg.layer_separation));
  REQUIRE(Y(alg, n[2]) == Catch::Approx(cfg.layer_separation));

  REQUIRE(X(alg, n[0]) == Catch::Approx((X(alg, n[1]) + X(alg, n[2])) / 2.0));
  REQUIRE(std::fabs(X(alg, n[1]) - X(alg, n[2])) >= cfg.min_separation - 1e-6);
}

TEST_CASE("Binary tree depth 2 is finite and level-correct",
         "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(7);
  auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(n[0], n[1]);
  g.AddEdge(n[0], n[2]);
  g.AddEdge(n[1], n[3]);
  g.AddEdge(n[1], n[4]);
  g.AddEdge(n[2], n[5]);
  g.AddEdge(n[2], n[6]);

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, n[0], &cfg);
  alg.PlaceAll();

  for (NodeID id : n) {
    REQUIRE(std::isfinite(X(alg, id)));
    REQUIRE(std::isfinite(Y(alg, id)));
  }

  REQUIRE(Y(alg, n[0]) == Catch::Approx(0.0));
  REQUIRE(Y(alg, n[1]) == Catch::Approx(cfg.layer_separation));
  REQUIRE(Y(alg, n[2]) == Catch::Approx(cfg.layer_separation));
  REQUIRE(Y(alg, n[3]) == Catch::Approx(2 * cfg.layer_separation));
  REQUIRE(Y(alg, n[4]) == Catch::Approx(2 * cfg.layer_separation));
  REQUIRE(Y(alg, n[5]) == Catch::Approx(2 * cfg.layer_separation));
  REQUIRE(Y(alg, n[6]) == Catch::Approx(2 * cfg.layer_separation));

  std::vector<NodeID> leaves = {n[3], n[4], n[5], n[6]};
  for (size_t i = 0; i < leaves.size(); i++)
    for (size_t j = i + 1; j < leaves.size(); j++)
      REQUIRE(std::fabs(X(alg, leaves[i]) - X(alg, leaves[j])) >= 1.0);
}

TEST_CASE("Path stays at constant x", "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(4);
  auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(n[0], n[1]);
  g.AddEdge(n[1], n[2]);
  g.AddEdge(n[2], n[3]);

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, n[0], &cfg);
  alg.PlaceAll();

  for (size_t i = 0; i < n.size(); i++) {
    REQUIRE(X(alg, n[i]) == Catch::Approx(X(alg, n[0])));
    REQUIRE(Y(alg, n[i]) == Catch::Approx(static_cast<double>(i) * cfg.layer_separation));
  }
}

TEST_CASE("Wider tree has no overlap among siblings or leaves",
         "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(6);
  auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(n[0], n[1]);
  g.AddEdge(n[0], n[2]);
  g.AddEdge(n[0], n[3]);
  g.AddEdge(n[2], n[4]);
  g.AddEdge(n[2], n[5]);

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, n[0], &cfg);
  alg.PlaceAll();

  std::vector<NodeID> children = {n[1], n[2], n[3]};
  for (size_t i = 0; i < children.size(); i++)
    for (size_t j = i + 1; j < children.size(); j++)
      REQUIRE(std::fabs(X(alg, children[i]) - X(alg, children[j])) >= 1.0);

  std::vector<NodeID> leaves = {n[1], n[3], n[4], n[5]};
  for (size_t i = 0; i < leaves.size(); i++)
    for (size_t j = i + 1; j < leaves.size(); j++)
      REQUIRE(std::fabs(X(alg, leaves[i]) - X(alg, leaves[j])) >= 1.0);
}

TEST_CASE("Skewed-then-branching chain keeps constant x until it branches",
         "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(6);
  auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(n[0], n[1]);
  g.AddEdge(n[1], n[2]);
  g.AddEdge(n[2], n[3]);
  g.AddEdge(n[3], n[4]);
  g.AddEdge(n[3], n[5]);

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, n[0], &cfg);
  alg.PlaceAll();

  for (size_t i = 0; i <= 3; i++) {
    REQUIRE(X(alg, n[i]) == Catch::Approx(X(alg, n[0])));
    REQUIRE(Y(alg, n[i]) == Catch::Approx(static_cast<double>(i) * cfg.layer_separation));
  }

  REQUIRE(std::fabs(X(alg, n[4]) - X(alg, n[5])) >= 1.0);
  REQUIRE(X(alg, n[3]) == Catch::Approx((X(alg, n[4]) + X(alg, n[5])) / 2.0));
  REQUIRE(Y(alg, n[4]) == Catch::Approx(4 * cfg.layer_separation));
  REQUIRE(Y(alg, n[5]) == Catch::Approx(4 * cfg.layer_separation));
}

TEST_CASE("Stepwise PlaceNext reaches the same terminal state as PlaceAll",
         "[layout][algorithms][reingold_tilford]") {
  Graph g = MakeDirectedGraph(7);
  auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(n[0], n[1]);
  g.AddEdge(n[0], n[2]);
  g.AddEdge(n[1], n[3]);
  g.AddEdge(n[1], n[4]);
  g.AddEdge(n[2], n[5]);
  g.AddEdge(n[2], n[6]);

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  cfg.step_animation_seconds = 0.01;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, n[0], &cfg);

  int guard = 0;
  while (!alg.Done() && guard++ < 100000)
    alg.PlaceNext();

  REQUIRE(alg.Done());

  // PlaceNext() only sets up the animation toward the final positions;
  // let it finish before checking where nodes actually ended up.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  alg.Tick();

  for (NodeID id : n) {
    REQUIRE(std::isfinite(X(alg, id)));
    REQUIRE(std::isfinite(Y(alg, id)));
  }

  std::vector<NodeID> leaves = {n[3], n[4], n[5], n[6]};
  for (size_t i = 0; i < leaves.size(); i++)
    for (size_t j = i + 1; j < leaves.size(); j++)
      REQUIRE(std::fabs(X(alg, leaves[i]) - X(alg, leaves[j])) >= 1.0);
}

namespace {

void CheckNoOverlapPerLevel(Graph &g, NodeID root) {
  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration cfg;
  ReingoldTilfordLayoutAlgorithm<Graph> alg(g, root, &cfg);
  alg.PlaceAll();

  std::vector<std::vector<NodeID>> levels;
  std::queue<std::pair<NodeID, size_t>> q;
  q.push({root, 0});
  while (!q.empty()) {
    auto [v, depth] = q.front();
    q.pop();
    if (levels.size() <= depth)
      levels.resize(depth + 1);
    levels[depth].push_back(v);
    for (const AdjEntry &adj : g.OutNeighbors(v))
      q.push({adj.other, depth + 1});
  }

  for (size_t depth = 0; depth < levels.size(); depth++) {
    const auto &level = levels[depth];
    double prevX = 0.0;
    for (size_t i = 0; i < level.size(); i++) {
      double x = X(alg, level[i]);
      double y = Y(alg, level[i]);
      REQUIRE(std::isfinite(x));
      REQUIRE(y == Catch::Approx(static_cast<double>(depth) * cfg.layer_separation));
      if (i > 0)
        REQUIRE(x > prevX);
      prevX = x;
    }
  }
}

} // namespace

TEST_CASE("Random trees have no same-depth overlap or order inversion",
         "[layout][algorithms][reingold_tilford]") {
  struct Edge { int from, to; };
  // A handful of fixed, irregular shapes instead of a random generator, to
  // keep this deterministic while still covering uneven branching/depth.
  std::vector<std::vector<Edge>> trees = {
      {{0, 1}, {0, 2}, {1, 3}, {3, 4}, {3, 5}, {2, 6}, {2, 7}, {6, 8}},
      {{0, 1}, {1, 2}, {2, 3}, {2, 4}, {0, 5}, {5, 6}, {5, 7}, {7, 8}, {7, 9}},
      {{0, 1}, {0, 2}, {0, 3}, {1, 4}, {2, 5}, {2, 6}, {3, 7}, {3, 8}, {3, 9}},
  };

  for (const auto &edges : trees) {
    int maxId = 0;
    for (const Edge &e : edges)
      maxId = std::max({maxId, e.from, e.to});

    Graph g = MakeDirectedGraph(maxId + 1);
    auto n = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
    for (const Edge &e : edges)
      g.AddEdge(n[e.from], n[e.to]);

    CheckNoOverlapPerLevel(g, n[0]);
  }
}
