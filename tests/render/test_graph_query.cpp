// ExtractFrameData/ReadIs3D translate a GraphLike graph's live component data
// into a flat FrameData buffer for the renderer, without touching any actual
// GPU/window state -- this is pure data transformation and testable without
// GVIZ_BUILD_RENDER.

#include "catch_amalgamated.hpp"
#include "render/graph_query.hpp"

#include "helpers/graph_helpers.hpp"

using test_helpers::MakeGraph;
using test_helpers::SetPosition;
using test_helpers::WithPositions;

TEST_CASE("ReadIs3D reflects the graph's DimensionResource", "[render][graph_query]") {
  Graph g2 = MakeGraph(1, 2);
  REQUIRE_FALSE(ReadIs3D(g2));

  Graph g3 = MakeGraph(1, 3);
  REQUIRE(ReadIs3D(g3));
}

TEST_CASE("ReadIs3D throws when DimensionResource hasn't been set", "[render][graph_query]") {
  Graph g;
  g.AddNode();
  REQUIRE_THROWS_AS(ReadIs3D(g), MissingResourceException<DimensionResource>);
}

TEST_CASE("ExtractFrameData is empty when there's no position pool",
         "[render][graph_query]") {
  Graph g = MakeGraph(3, 2);
  FrameData fd;
  ExtractFrameData(g, false, fd);

  REQUIRE(fd.positions.empty());
  REQUIRE_FALSE(fd.bboxValid);
}

TEST_CASE("ExtractFrameData emits one xyz triple and one radius per node",
         "[render][graph_query]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {1.0, 2.0});
  SetPosition(positions, 1, {3.0, 4.0});

  FrameData fd;
  ExtractFrameData(g, /*is3D=*/false, fd);

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
         "[render][graph_query]") {
  Graph g = MakeGraph(1, 3);
  auto *positions = WithPositions(g, 3);
  SetPosition(positions, 0, {1.0, 2.0, 3.0});

  FrameData fd3D;
  ExtractFrameData(g, /*is3D=*/true, fd3D);
  REQUIRE(fd3D.positions[2] == Catch::Approx(3.0f));

  FrameData fd2D;
  ExtractFrameData(g, /*is3D=*/false, fd2D);
  REQUIRE(fd2D.positions[2] == Catch::Approx(0.0f));
}

TEST_CASE("Without a VisibleNodesResource, every node is treated as visible",
         "[render][graph_query]") {
  Graph g = MakeGraph(2, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {10.0, 10.0});

  FrameData fd;
  ExtractFrameData(g, false, fd);

  REQUIRE(fd.nodeIds.size() == 2);
  REQUIRE(fd.bboxValid);
  REQUIRE(fd.bboxMin[0] == Catch::Approx(0.0f));
  REQUIRE(fd.bboxMax[0] == Catch::Approx(10.0f));
}

TEST_CASE("With a VisibleNodesResource, only visible nodes contribute to "
         "nodeIds and the bounding box",
         "[render][graph_query]") {
  Graph g = MakeGraph(3, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {0.0, 0.0});
  SetPosition(positions, 1, {100.0, 100.0}); // hidden -- should not skew bbox
  SetPosition(positions, 2, {5.0, 5.0});

  auto &visible = g.SetResource<VisibleNodesResource>(3, 0);
  visible.Set(DenseNodeID(0));
  visible.Set(DenseNodeID(2));

  FrameData fd;
  ExtractFrameData(g, false, fd);

  REQUIRE(fd.nodeIds.size() == 2);
  REQUIRE(fd.bboxValid);
  REQUIRE(fd.bboxMax[0] == Catch::Approx(5.0f));
  REQUIRE(fd.bboxMax[1] == Catch::Approx(5.0f));
}

TEST_CASE("Edges are only emitted when both endpoints are visible",
         "[render][graph_query]") {
  Graph g = MakeGraph(3, 2);
  WithPositions(g, 2);
  g.AddEdge(NodeID(0), NodeID(1));
  g.AddEdge(NodeID(1), NodeID(2));

  auto &visible = g.SetResource<VisibleNodesResource>(3, 0);
  visible.Set(DenseNodeID(0));
  visible.Set(DenseNodeID(1));
  // node 2 stays hidden

  FrameData fd;
  ExtractFrameData(g, false, fd);

  REQUIRE(fd.edges.size() == 2); // one edge (0,1) -> 2 entries
  REQUIRE(fd.edges[0] == 0);
  REQUIRE(fd.edges[1] == 1);
}

TEST_CASE("ExtractFrameData reuses out's storage: a second call clears prior "
         "contents rather than appending",
         "[render][graph_query]") {
  Graph g = MakeGraph(1, 2);
  auto *positions = WithPositions(g, 2);
  SetPosition(positions, 0, {1.0, 1.0});

  FrameData fd;
  ExtractFrameData(g, false, fd);
  REQUIRE(fd.positions.size() == 3);

  ExtractFrameData(g, false, fd);
  REQUIRE(fd.positions.size() == 3); // not 6
}
