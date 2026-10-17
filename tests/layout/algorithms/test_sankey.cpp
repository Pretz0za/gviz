#include "catch_amalgamated.hpp"
#include "layout/algorithms/sankey.hpp"
#include "layout/components/cubic_bezier.hpp"
#include "layout/components/half_width.hpp"
#include "layout/components/sankey_node.hpp"
#include <cmath>
#include <vector>

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;

TEST_CASE("Linear chain is assigned one node per layer",
         "[layout][algorithms][sankey]") {
  Graph g = MakeGraph(3);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  g.AddEdge(nodes[0], nodes[1], 2.0f);
  g.AddEdge(nodes[1], nodes[2], 2.0f);

  SankeyLayoutAlgorithm<Graph>::Configuration config;
  SankeyLayoutAlgorithm<Graph> alg(g, &config);
  alg.Layout();

  REQUIRE(alg.LayerCount() == 3);

  auto *info = g.NodeSpace().GetPool<SankeyNodeDecorator>();
  REQUIRE(info->Find(0)->layer == 0);
  REQUIRE(info->Find(1)->layer == 1);
  REQUIRE(info->Find(2)->layer == 2);

  auto *positions = g.NodeSpace().GetPool<PositionComponent>();
  REQUIRE(positions->Find(1)->pos[0] > positions->Find(0)->pos[0]);
  REQUIRE(positions->Find(2)->pos[0] > positions->Find(1)->pos[0]);
}

TEST_CASE("Edges receive a bezier control-point curve with positive width",
         "[layout][algorithms][sankey]") {
  Graph g = MakeGraph(2);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  EdgeID e = g.AddEdge(nodes[0], nodes[1], 4.0f);

  SankeyLayoutAlgorithm<Graph>::Configuration config;
  SankeyLayoutAlgorithm<Graph> alg(g, &config);
  alg.Layout();

  auto *curves = g.EdgeSpace().GetPool<CubicBezierComponent>();
  auto *halfWidths = g.EdgeSpace().GetPool<HalfWidthComponent>();
  CubicBezierComponent *curve = curves->Find(g.MapToDense(e).Raw());
  HalfWidthComponent *halfWidth = halfWidths->Find(g.MapToDense(e).Raw());

  REQUIRE(halfWidth->halfWidth > 0.0);
  REQUIRE(curve->p3[0] > curve->p0[0]);
  REQUIRE(curve->p1[1] == Catch::Approx(curve->p0[1]));
  REQUIRE(curve->p2[1] == Catch::Approx(curve->p3[1]));
}

TEST_CASE("Diverging flow splits a node's outgoing ports without overlap",
         "[layout][algorithms][sankey]") {
  Graph g = MakeGraph(3);
  auto nodes = std::vector<NodeID>(g.Nodes().begin(), g.Nodes().end());
  EdgeID a = g.AddEdge(nodes[0], nodes[1], 3.0f);
  EdgeID b = g.AddEdge(nodes[0], nodes[2], 5.0f);

  SankeyLayoutAlgorithm<Graph>::Configuration config;
  SankeyLayoutAlgorithm<Graph> alg(g, &config);
  alg.Layout();

  auto *curves = g.EdgeSpace().GetPool<CubicBezierComponent>();
  auto *halfWidths = g.EdgeSpace().GetPool<HalfWidthComponent>();
  CubicBezierComponent *ca = curves->Find(g.MapToDense(a).Raw());
  CubicBezierComponent *cb = curves->Find(g.MapToDense(b).Raw());

  double aY = ca->p0[1];
  double bY = cb->p0[1];
  double aHalf = halfWidths->Find(g.MapToDense(a).Raw())->halfWidth;
  double bHalf = halfWidths->Find(g.MapToDense(b).Raw())->halfWidth;

  REQUIRE(std::abs(aY - bY) >= aHalf + bHalf - 1e-6);
}
