#include "catch_amalgamated.hpp"
#include "layout/algorithms/tree_convex.hpp"
#include "layout/components/radial_edge_decorator.hpp"
#include <cmath>
#include <numbers>

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;

namespace {
double Resolution(Graph &g, bool respectEmbedding = true) {
  TreeConvexLayoutAlgorithm<Graph> alg(g, respectEmbedding);
  alg.Layout();
  return alg.AngularResolution();
}
} // namespace

TEST_CASE("Single node places at origin", "[layout][algorithms][tree_convex]") {
  Graph g = MakeGraph(1, 2);
  TreeConvexLayoutAlgorithm<Graph> alg(g, true);
  alg.Layout();

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  REQUIRE(positions->Find(0)->pos[0] == Catch::Approx(0.0));
  REQUIRE(positions->Find(0)->pos[1] == Catch::Approx(0.0));
}

TEST_CASE("Plain path achieves resolution pi", "[layout][algorithms][tree_convex]") {
  Graph g = MakeGraph(5, 2);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  for (size_t i = 0; i + 1 < nodes.size(); i++)
    g.AddUndirectedEdge(nodes[i], nodes[i + 1]);

  REQUIRE(Resolution(g) == Catch::Approx(std::numbers::pi).epsilon(1e-6));
}

TEST_CASE("Plain rake achieves resolution pi/2", "[layout][algorithms][tree_convex]") {
  Graph g = MakeGraph(4, 2);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddUndirectedEdge(nodes[0], nodes[1]);
  g.AddUndirectedEdge(nodes[0], nodes[2]);
  g.AddUndirectedEdge(nodes[0], nodes[3]);

  double res = Resolution(g);
  REQUIRE(res == Catch::Approx(1.5707963267948966).epsilon(1e-6));
}

TEST_CASE("Triple rake achieves resolution pi/3", "[layout][algorithms][tree_convex]") {
  Graph g = MakeGraph(10, 2);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddUndirectedEdge(nodes[0], nodes[1]);
  g.AddUndirectedEdge(nodes[0], nodes[2]);
  g.AddUndirectedEdge(nodes[0], nodes[3]);

  g.AddUndirectedEdge(nodes[1], nodes[4]);
  g.AddUndirectedEdge(nodes[1], nodes[5]);

  g.AddUndirectedEdge(nodes[2], nodes[6]);
  g.AddUndirectedEdge(nodes[2], nodes[7]);

  g.AddUndirectedEdge(nodes[3], nodes[8]);
  g.AddUndirectedEdge(nodes[3], nodes[9]);

  double res = Resolution(g);
  REQUIRE(res == Catch::Approx(1.0471975511965976).epsilon(1e-6));
}

TEST_CASE("Bent-path children never collapse onto a duplicate sibling slope",
         "[layout][algorithms][tree_convex]") {
  Graph g = MakeGraph(11, 2);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddUndirectedEdge(nodes[0], nodes[1]);
  g.AddUndirectedEdge(nodes[0], nodes[2]);
  g.AddUndirectedEdge(nodes[0], nodes[3]);
  g.AddUndirectedEdge(nodes[1], nodes[4]);
  g.AddUndirectedEdge(nodes[1], nodes[5]);
  g.AddUndirectedEdge(nodes[2], nodes[6]);
  g.AddUndirectedEdge(nodes[2], nodes[7]);
  g.AddUndirectedEdge(nodes[2], nodes[8]);
  g.AddUndirectedEdge(nodes[3], nodes[9]);
  g.AddUndirectedEdge(nodes[3], nodes[10]);

  TreeConvexLayoutAlgorithm<Graph> alg(g, false);
  alg.Layout();

  auto *edgePool = g.EdgeSpace().GetPool<RadialEdgeDecorator>();
  bool anyDuplicate = false;
  for (NodeID v : g.Nodes()) {
    std::vector<double> slopes;
    for (const AdjEntry &adj : g.OutNeighbors(v))
      slopes.push_back(edgePool->Find(adj.edge.Raw())->slope);
    for (size_t a = 0; a < slopes.size(); a++)
      for (size_t b = a + 1; b < slopes.size(); b++)
        if (std::fabs(slopes[a] - slopes[b]) < 1e-9)
          anyDuplicate = true;
  }
  REQUIRE_FALSE(anyDuplicate);
}

TEST_CASE("General tree produces a positive resolution and valid positions",
         "[layout][algorithms][tree_convex]") {
  Graph g = MakeGraph(13, 2);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddUndirectedEdge(nodes[0], nodes[1]);
  g.AddUndirectedEdge(nodes[1], nodes[2]);
  g.AddUndirectedEdge(nodes[1], nodes[3]);
  g.AddUndirectedEdge(nodes[1], nodes[4]);

  g.AddUndirectedEdge(nodes[0], nodes[5]);
  g.AddUndirectedEdge(nodes[5], nodes[6]);
  g.AddUndirectedEdge(nodes[5], nodes[7]);
  g.AddUndirectedEdge(nodes[5], nodes[8]);

  g.AddUndirectedEdge(nodes[0], nodes[9]);
  g.AddUndirectedEdge(nodes[9], nodes[10]);
  g.AddUndirectedEdge(nodes[9], nodes[11]);
  g.AddUndirectedEdge(nodes[9], nodes[12]);

  TreeConvexLayoutAlgorithm<Graph> alg(g, false);
  alg.Layout();

  REQUIRE(alg.AngularResolution() > 0.0);
  REQUIRE(alg.AngularResolution() <= std::numbers::pi);

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  bool anyNonZero = false;
  for (uint32_t i = 0; i < 13; i++) {
    PositionComponent *p = positions->Find(i);
    if (p->pos[0] != 0.0 || p->pos[1] != 0.0)
      anyNonZero = true;
  }
  REQUIRE(anyNonZero);
}
