#include "ds/state_machine.hpp"
#include "graph/graph.hpp"
#include "layout/algorithms/tree_convex.hpp"
#include "layout/types.hpp"
#include "render/renderer.hpp"
#include <random>
#include <vector>

static void AddRandomChildren(Graph &g, NodeID root, int depth, int maxDepth,
                              int branchingFactor, bool exact,
                              std::mt19937 &rng) {
  if (depth >= maxDepth)
    return;

  int count = branchingFactor;
  if (!exact && branchingFactor >= 2) {
    std::uniform_int_distribution<int> dist(2, branchingFactor);
    count = dist(rng);
  }

  for (int i = 0; i < count; i++) {
    NodeID child = g.AddNode();
    g.AddUndirectedEdge(root, child);
    AddRandomChildren(g, child, depth + 1, maxDepth, branchingFactor, exact,
                      rng);
  }
}

static Graph BuildRandomTree(int maxDepth, int branchingFactor, bool exact,
                             unsigned int seed) {
  Graph g;
  NodeID root = g.AddNode();
  std::mt19937 rng(seed);
  AddRandomChildren(g, root, 0, maxDepth, branchingFactor, exact, rng);
  return g;
}

int main() {
  Graph g = BuildRandomTree(8, 4, false, std::random_device{}());
  g.SetResource<DimensionResource>(DimensionResource::D2);

  TreeConvexLayoutAlgorithm<Graph>::Configuration layoutConfig;
  TreeConvexLayoutAlgorithm<Graph> layout(g, false, &layoutConfig);
  layout.Tick();

  Renderer renderer(1280, 720, "gviz treeDemo", g);
  renderer.SetLockToFit(true);

#ifdef GVIZ_DEBUG_CHARTS
  renderer.DebugConfigs().Add("Tree Convex", layoutConfig);
#endif

  std::mt19937 rng(std::random_device{}());
  StateMachine<EdgeLengthMode> *edgeLengthMode =
      g.GetResource<StateMachine<EdgeLengthMode>>();

  while (renderer.Frame(g)) {
    layout.Tick();

    if (renderer.KeyPressed('T')) {
      std::vector<EdgeLengthMode> transitions = edgeLengthMode->Transitions();
      std::uniform_int_distribution<size_t> dist(0, transitions.size() - 1);
      layout.TransitionTo(transitions[dist(rng)], 0.6);
    }
  }

  return 0;
}
