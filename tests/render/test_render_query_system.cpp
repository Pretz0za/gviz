// RenderQuerySystem/ReadIs3D translate a GraphLike graph's live component
// data into a flat FrameData buffer for the renderer, without touching any
// actual GPU/window state -- this is pure data transformation and testable
// without GVIZ_BUILD_RENDER.

#include "catch_amalgamated.hpp"
#include "layout/components/cubic_bezier.hpp"
#include "layout/components/half_width.hpp"
#include "layout/components/radius.hpp"
#include "render/render_query_system.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::SetPosition;
using test_helpers::WithPositions;

TEST_CASE("ReadIs3D reflects the graph's DimensionResource", "[render][render_query_system]") {
  Graph g2 = MakeGraph(1, 2);
  REQUIRE_FALSE(ReadIs3D(g2));

  Graph g3 = MakeGraph(1, 3);
  REQUIRE(ReadIs3D(g3));
}

TEST_CASE("ReadIs3D throws when DimensionResource hasn't been set", "[render][render_query_system]") {
  Graph g;
  g.AddNode();
  REQUIRE_THROWS_AS(ReadIs3D(g), MissingResourceException<DimensionResource>);
}

TEST_CASE("ExtractFrameData is empty when there's no position pool",
         "[render][render_query_system]") {
  Graph g = MakeGraph(3, 2);
  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.positions.empty());
  REQUIRE_FALSE(fd.bboxValid);
}

TEST_CASE("ExtractFrameData emits one xyz triple and one radius per node",
         "[render][render_query_system]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {1.0, 2.0});
  SetPosition(positions, 1, {3.0, 4.0});

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(/*is3D=*/false, fd);

  REQUIRE(fd.positions.size() == 6); // 2 nodes * 3 floats each
  REQUIRE(fd.positions[0] == Catch::Approx(1.0f));
  REQUIRE(fd.positions[1] == Catch::Approx(2.0f));
  REQUIRE(fd.positions[2] == Catch::Approx(0.0f)); // z zeroed in 2D mode
  REQUIRE(fd.positions[3] == Catch::Approx(3.0f));
  REQUIRE(fd.positions[4] == Catch::Approx(4.0f));

  REQUIRE(fd.radii.size() == 2);
  REQUIRE(fd.radii[0] == Catch::Approx(static_cast<float>(DEFAULT_RADIUS)));
}

TEST_CASE("ExtractFrameData includes the z coordinate only when is3D is true",
         "[render][render_query_system]") {
  Graph g = MakeGraph(1, 3);
  auto *positions = WithPositions(g, 3);
  SetPosition(positions, 0, {1.0, 2.0, 3.0});

  RenderQuerySystem<Graph> query(g);

  FrameData fd3D;
  query.ExtractFrameData(/*is3D=*/true, fd3D);
  REQUIRE(fd3D.positions[2] == Catch::Approx(3.0f));

  FrameData fd2D;
  query.ExtractFrameData(/*is3D=*/false, fd2D);
  REQUIRE(fd2D.positions[2] == Catch::Approx(0.0f));
}

TEST_CASE("Without a VisibleNodesResource, every node is treated as visible",
         "[render][render_query_system]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 10.0});

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.nodeIds.size() == 2);
  REQUIRE(fd.bboxValid);
  REQUIRE(fd.bboxMin[0] == Catch::Approx(0.0f));
  REQUIRE(fd.bboxMax[0] == Catch::Approx(10.0f));
}

TEST_CASE("With a VisibleNodesResource, only visible nodes contribute to "
         "nodeIds and the bounding box",
         "[render][render_query_system]") {
  Graph g = MakeGraph(3, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {100.0, 100.0}); // hidden -- should not skew bbox
  SetPosition(positions, 2, {5.0, 5.0});

  auto &visible = g.SetResource<VisibleNodesResource>(3, 0);
  visible.Set(DenseNodeID(0));
  visible.Set(DenseNodeID(2));

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.nodeIds.size() == 2);
  REQUIRE(fd.bboxValid);
  REQUIRE(fd.bboxMax[0] == Catch::Approx(5.0f));
  REQUIRE(fd.bboxMax[1] == Catch::Approx(5.0f));
}

TEST_CASE("Edges are only emitted when both endpoints are visible",
         "[render][render_query_system]") {
  Graph g = MakeGraph(3, 2);
  WithPositions(g, 2);
  g.AddEdge(NodeID(0), NodeID(1));
  g.AddEdge(NodeID(1), NodeID(2));

  auto &visible = g.SetResource<VisibleNodesResource>(3, 0);
  visible.Set(DenseNodeID(0));
  visible.Set(DenseNodeID(1));
  // node 2 stays hidden

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.edges.size() == 2); // one edge (0,1) -> 2 entries
  REQUIRE(fd.edges[0] == 0);
  REQUIRE(fd.edges[1] == 1);
}

TEST_CASE("ExtractFrameData reuses out's storage: a second call clears prior "
         "contents rather than appending",
         "[render][render_query_system]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {1.0, 1.0});

  RenderQuerySystem<Graph> query(g);

  FrameData fd;
  query.ExtractFrameData(false, fd);
  REQUIRE(fd.positions.size() == 3);

  query.ExtractFrameData(false, fd);
  REQUIRE(fd.positions.size() == 3); // not 6
}

TEST_CASE("An edge with a CubicBezierComponent and HalfWidthComponent is "
         "rendered as a curve, not a straight edge",
         "[render][render_query_system]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  EdgeID e = g.AddEdge(NodeID(0), NodeID(1));

  auto *curves = g.EdgeSpace().SetPool<CubicBezierComponent>();
  auto *halfWidths = g.EdgeSpace().SetPool<HalfWidthComponent>();
  CubicBezierComponent *curve = curves->Find(g.MapToDense(e).Raw());
  curve->p0[0] = 0.0; curve->p0[1] = 0.0;
  curve->p1[0] = 3.0; curve->p1[1] = 0.0;
  curve->p2[0] = 7.0; curve->p2[1] = 0.0;
  curve->p3[0] = 10.0; curve->p3[1] = 0.0;
  halfWidths->Find(g.MapToDense(e).Raw())->halfWidth = 2.0;

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.edges.empty());
  REQUIRE(fd.bezierControlPoints.size() == 12); // 4 control points * xyz
  REQUIRE(fd.halfWidths.size() == 1);
  REQUIRE(fd.edgeColors.size() == 1);

  REQUIRE(fd.bezierControlPoints[3] == Catch::Approx(3.0f));
  REQUIRE(fd.bezierControlPoints[9] == Catch::Approx(10.0f));
  REQUIRE(fd.halfWidths[0] == Catch::Approx(2.0f));
}

TEST_CASE("An edge with a CubicBezierComponent but no per-edge half-width "
         "still renders as a curve, using a default half-width",
         "[render][render_query_system]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  EdgeID e = g.AddEdge(NodeID(0), NodeID(1));

  auto *curves = g.EdgeSpace().SetPool<CubicBezierComponent>();
  CubicBezierComponent *curve = curves->Find(g.MapToDense(e).Raw());
  curve->p0[0] = 0.0; curve->p0[1] = 0.0;
  curve->p1[0] = 3.0; curve->p1[1] = 0.0;
  curve->p2[0] = 7.0; curve->p2[1] = 0.0;
  curve->p3[0] = 10.0; curve->p3[1] = 0.0;

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.edges.empty());
  REQUIRE(fd.bezierControlPoints.size() == 12);
  REQUIRE(fd.halfWidths.size() == 1);
  REQUIRE(fd.halfWidths[0] == Catch::Approx(static_cast<float>(DEFAULT_HALF_WIDTH)));
}

TEST_CASE("A straight edge's per-edge half-width is carried into FrameData "
         "even without a bezier curve",
         "[render][render_query_system]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  EdgeID e = g.AddEdge(NodeID(0), NodeID(1));

  auto *halfWidths = g.EdgeSpace().SetPool<HalfWidthComponent>();
  halfWidths->Find(g.MapToDense(e).Raw())->halfWidth = 4.0;

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.bezierControlPoints.empty());
  REQUIRE(fd.edges.size() == 2);
  REQUIRE(fd.halfWidths.size() == 1);
  REQUIRE(fd.halfWidths[0] == Catch::Approx(4.0f));
}

TEST_CASE("Curved and straight edges coexist, and both contribute to the "
         "bounding box",
         "[render][render_query_system]") {
  Graph g = MakeGraph(4, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  SetPosition(positions, 2, {0.0, 0.0});
  SetPosition(positions, 3, {1.0, 1.0});

  EdgeID curvedEdge = g.AddEdge(NodeID(0), NodeID(1));
  g.AddEdge(NodeID(2), NodeID(3));

  auto *curves = g.EdgeSpace().SetPool<CubicBezierComponent>();
  auto *halfWidths = g.EdgeSpace().SetPool<HalfWidthComponent>();
  CubicBezierComponent *curve = curves->Find(g.MapToDense(curvedEdge).Raw());
  curve->p0[0] = 0.0; curve->p0[1] = 0.0;
  curve->p1[0] = 3.0; curve->p1[1] = 50.0;
  curve->p2[0] = 7.0; curve->p2[1] = 50.0;
  curve->p3[0] = 10.0; curve->p3[1] = 0.0;
  halfWidths->Find(g.MapToDense(curvedEdge).Raw())->halfWidth = 1.0;

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.edges.size() == 2); // the (2,3) edge only
  REQUIRE(fd.halfWidths.size() == 2); // one per edge: the curve, then the straight edge
  REQUIRE(fd.bboxValid);
  REQUIRE(fd.bboxMax[1] >= 50.0f);
}

TEST_CASE("A node's rect half-height is derived from the thickness of its "
         "incident curved edges, not stored",
         "[render][render_query_system]") {
  Graph g = MakeGraph(3, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, -5.0});
  SetPosition(positions, 2, {10.0, 5.0});

  EdgeID e0 = g.AddEdge(NodeID(0), NodeID(1));
  EdgeID e1 = g.AddEdge(NodeID(0), NodeID(2));

  auto *curves = g.EdgeSpace().SetPool<CubicBezierComponent>();
  auto *halfWidths = g.EdgeSpace().SetPool<HalfWidthComponent>();
  for (EdgeID e : {e0, e1}) {
    CubicBezierComponent *curve = curves->Find(g.MapToDense(e).Raw());
    curve->p0[0] = 0.0; curve->p0[1] = 0.0;
    curve->p1[0] = 3.0; curve->p1[1] = 0.0;
    curve->p2[0] = 7.0; curve->p2[1] = 0.0;
    curve->p3[0] = 10.0; curve->p3[1] = 0.0;
  }
  halfWidths->Find(g.MapToDense(e0).Raw())->halfWidth = 3.0;
  halfWidths->Find(g.MapToDense(e1).Raw())->halfWidth = 5.0;

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.nodeRectHalfHeights.size() == 3); // dense, 1 per node
  REQUIRE(fd.nodeRectHalfHeights[0] == Catch::Approx(8.0f)); // 3 + 5 outgoing
  REQUIRE(fd.nodeRectHalfHeights[1] == Catch::Approx(3.0f)); // incoming only
  REQUIRE(fd.nodeRectHalfHeights[2] == Catch::Approx(5.0f)); // incoming only
}

TEST_CASE("Without a HalfWidthComponent pool, nodeRectHalfHeights stays "
         "empty so every node renders as a circle",
         "[render][render_query_system]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 0.0});
  g.AddEdge(NodeID(0), NodeID(1));

  FrameData fd;
  RenderQuerySystem<Graph>(g).ExtractFrameData(false, fd);

  REQUIRE(fd.nodeRectHalfHeights.empty());
}
