#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "layout/algorithms/spring.hpp"
#include "layout/components/radius.hpp"
#include "layout/placement/randomized.hpp"
#include "render/renderer.hpp"
#include <array>
#include <cstdint>
#include <vector>

static Graph BuildGrid(int rows, int cols, std::vector<std::vector<NodeID>> &nodes) {
  Graph g;
  nodes.assign(static_cast<size_t>(rows), std::vector<NodeID>(static_cast<size_t>(cols)));
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
      nodes[static_cast<size_t>(r)][static_cast<size_t>(c)] = g.AddNode();

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (c + 1 < cols)
        g.AddUndirectedEdge(nodes[static_cast<size_t>(r)][static_cast<size_t>(c)],
                            nodes[static_cast<size_t>(r)][static_cast<size_t>(c + 1)]);
      if (r + 1 < rows)
        g.AddUndirectedEdge(nodes[static_cast<size_t>(r)][static_cast<size_t>(c)],
                            nodes[static_cast<size_t>(r + 1)][static_cast<size_t>(c)]);
    }
  }
  return g;
}

int main() {
  const int rows = 6;
  const int cols = 6;
  const double spacing = 40.0;
  const double squareSide = (cols - 1) * spacing;

  std::vector<std::vector<NodeID>> nodes;
  Graph g = BuildGrid(rows, cols, nodes);

  SpringLayoutAlgorithm<Graph>::Configuration layoutConfig;
  layoutConfig.stiffness = 2.0;
  layoutConfig.restLength = spacing;
  layoutConfig.damping = 1.5;
  layoutConfig.timeStep = 0.08;
  SpringLayoutAlgorithm<Graph> layout(g, &layoutConfig);
  DecoratedGraph<Graph> &dg = layout.Decorated();

  PositionRandomized<Graph>::Configuration randomConfig;
  randomConfig.boundingBoxWidth = static_cast<uint32_t>(squareSide * 0.6);
  PositionRandomized<Graph> randomizer(dg, &randomConfig);
  randomizer.PlaceAll();

  NodeID topLeft = nodes[0][0];
  NodeID topRight = nodes[0][static_cast<size_t>(cols - 1)];
  NodeID bottomLeft = nodes[static_cast<size_t>(rows - 1)][0];
  NodeID bottomRight = nodes[static_cast<size_t>(rows - 1)][static_cast<size_t>(cols - 1)];

  std::array<double, 2> topLeftPos{0.0, 0.0};
  std::array<double, 2> topRightPos{squareSide, 0.0};
  std::array<double, 2> bottomLeftPos{0.0, squareSide};
  std::array<double, 2> bottomRightPos{squareSide, squareSide};
  layout.SetPosition(topLeft, topLeftPos.data());
  layout.SetPosition(topRight, topRightPos.data());
  layout.SetPosition(bottomLeft, bottomLeftPos.data());
  layout.SetPosition(bottomRight, bottomRightPos.data());

  layout.Fix(topLeft);
  layout.Fix(topRight);
  layout.Fix(bottomLeft);
  layout.Fix(bottomRight);

  auto *radii = dg.nodeSpace.SetPool<RadiusComponent>(MANUAL);
  for (uint32_t i = 0; i < radii->Size(); i++)
    radii->Data()[i].radius = 6.0;

  Renderer renderer(1280, 720, "gviz springDemo", dg);
  renderer.SetLockToFit(true);
  renderer.SetNodeColor(0.95f, 0.55f, 0.15f, 1.0f);
  renderer.SetEdgeColor(0.3f, 0.55f, 0.85f, 0.55f);

#ifdef GVIZ_DEBUG_CHARTS
  renderer.DebugConfigs().Add("Spring Layout", layoutConfig);
#endif

  NodeID center = nodes[static_cast<size_t>(rows / 2)][static_cast<size_t>(cols / 2)];

  while (renderer.Frame(dg)) {
    layout.Tick();

    if (renderer.KeyPressed('F')) {
      if (layout.IsFixed(center))
        layout.Unfix(center);
      else
        layout.Fix(center);
    }
  }

  return 0;
}
