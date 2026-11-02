#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "layout/algorithms/reingold_tilford.hpp"
#include "render/renderer.hpp"
#include <cstdio>
#include <random>

static void AddRandomChildren(Graph &g, NodeID parent, int depth, int maxDepth,
                              int branchingFactor, std::mt19937 &rng) {
  if (depth >= maxDepth)
    return;

  std::uniform_int_distribution<int> dist(1, branchingFactor);
  int count = dist(rng);
  for (int i = 0; i < count; i++) {
    NodeID child = g.AddNode();
    g.AddEdge(parent, child);
    AddRandomChildren(g, child, depth + 1, maxDepth, branchingFactor, rng);
  }
}

static Graph BuildRandomTree(NodeID &root, int maxDepth, int branchingFactor,
                             unsigned int seed) {
  Graph g(Graph::GraphFlags::Directed);
  root = g.AddNode();
  std::mt19937 rng(seed);
  AddRandomChildren(g, root, 0, maxDepth, branchingFactor, rng);
  return g;
}

int main() {
  NodeID root;
  Graph g = BuildRandomTree(root, 5, 3, std::random_device{}());

  ReingoldTilfordLayoutAlgorithm<Graph>::Configuration layoutConfig;
  ReingoldTilfordLayoutAlgorithm<Graph> layout(g, root, &layoutConfig);

  auto &dg = layout.Decorated();
  Renderer renderer(1280, 720, "gviz rtDemo", dg);
  renderer.SetLockToFit(true);
  renderer.SetNodeColor(0.95f, 0.55f, 0.15f, 1.0f);
  renderer.SetEdgeColor(0.3f, 0.55f, 0.85f, 0.55f);

#ifdef GVIZ_DEBUG_CHARTS
  renderer.DebugConfigs().Add("Reingold-Tilford", layoutConfig);
#endif

  printf("gviz rtDemo controls:\n");
  printf("  N - advance one step (PlaceNext)\n");
  printf("  A - place the rest of the tree instantly (PlaceAll)\n");

  while (renderer.Frame(dg)) {
    layout.Tick();

    if (renderer.KeyPressed('N'))
      layout.PlaceNext();

    if (renderer.KeyPressed('A'))
      layout.PlaceAll();
  }

  return 0;
}
