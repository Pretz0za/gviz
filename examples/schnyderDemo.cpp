#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "layout/components/radius.hpp"
#include "layout/planarity/schnyder_layout.hpp"
#include "render/renderer.hpp"
#include <array>
#include <cstdint>
#include <random>
#include <vector>

static Graph BuildRandomPlanarGraph(int n, double keepProbability, unsigned seed) {
  Graph g;
  NodeID a = g.AddNode(), b = g.AddNode(), c = g.AddNode();
  g.AddEdge(a, b);
  g.AddEdge(b, c);
  g.AddEdge(c, a);

  std::vector<std::array<NodeID, 3>> faces = {{a, b, c}};
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> coin(0.0, 1.0);

  for (int i = 3; i < n; i++) {
    std::uniform_int_distribution<size_t> pickFace(0, faces.size() - 1);
    size_t idx = pickFace(rng);
    std::array<NodeID, 3> face = faces[idx];
    faces.erase(faces.begin() + static_cast<long>(idx));

    NodeID v = g.AddNode();

    std::uniform_int_distribution<int> pickAnchor(0, 2);
    int anchor = pickAnchor(rng);
    for (int k = 0; k < 3; k++) {
      if (k == anchor || coin(rng) < keepProbability)
        g.AddEdge(v, face[static_cast<size_t>(k)]);
    }

    faces.push_back({face[0], face[1], v});
    faces.push_back({face[1], face[2], v});
    faces.push_back({face[2], face[0], v});
  }

  return g;
}

int main() {
  Graph g = BuildRandomPlanarGraph(22, 0.55, std::random_device{}());

  SchnyderLayoutAlgorithm<Graph>::Configuration layoutConfig;
  layoutConfig.scale = 600.0;
  SchnyderLayoutAlgorithm<Graph> layout(g, &layoutConfig);
  layout.Tick();

  DecoratedGraph<Graph> &dg = layout.Decorated();
  auto *radii = dg.nodeSpace.SetPool<RadiusComponent>(MANUAL);
  for (uint32_t i = 0; i < radii->Size(); i++)
    radii->Data()[i].radius = 8.0;

  Renderer renderer(1280, 720, "gviz schnyderDemo", dg);
  renderer.SetLockToFit(true);
  renderer.SetNodeColor(0.95f, 0.55f, 0.15f, 1.0f);
  renderer.SetEdgeColor(0.3f, 0.55f, 0.85f, 0.55f);

#ifdef GVIZ_DEBUG_CHARTS
  renderer.DebugConfigs().Add("Schnyder Layout", layoutConfig);
#endif

  while (renderer.Frame(dg)) {
    layout.Tick();

    if (renderer.KeyPressed('R'))
      layout.RandomizeRoots();
  }

  return 0;
}
