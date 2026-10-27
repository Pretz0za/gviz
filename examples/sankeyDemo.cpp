#include "ds/flags.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "layout/algorithms/sankey.hpp"
#include "layout/components/radius.hpp"
#include "render/components/color.hpp"
#include "render/renderer.hpp"
#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

static uint32_t PackColor(float r, float g, float b, float a) {
  auto c = [](float v) { return static_cast<uint32_t>(v * 255.0f) & 0xFFu; };
  return (c(r) << 24) | (c(g) << 16) | (c(b) << 8) | c(a);
}

static std::vector<std::pair<int, int>> BuildBipartiteEdges(std::mt19937 &rng,
                                                            int leftCount,
                                                            int rightCount,
                                                            double extraProb) {
  std::uniform_int_distribution<int> leftPick(0, leftCount - 1);
  std::uniform_int_distribution<int> rightPick(0, rightCount - 1);
  std::uniform_real_distribution<double> prob(0.0, 1.0);

  std::vector<std::pair<int, int>> edges;
  for (int r = 0; r < rightCount; r++)
    edges.push_back({leftPick(rng), r});

  std::vector<bool> leftHasOut(leftCount, false);
  for (auto &e : edges)
    leftHasOut[e.first] = true;
  for (int l = 0; l < leftCount; l++)
    if (!leftHasOut[l])
      edges.push_back({l, rightPick(rng)});

  for (int l = 0; l < leftCount; l++)
    for (int r = 0; r < rightCount; r++)
      if (prob(rng) < extraProb)
        edges.push_back({l, r});

  std::ranges::sort(edges);
  edges.erase(std::ranges::unique(edges).begin(), edges.end());
  return edges;
}

static std::vector<double> RandomPartition(std::mt19937 &rng, size_t count,
                                           double total) {
  std::uniform_real_distribution<double> unit(0.1, 1.0);
  std::vector<double> parts(count);
  double sum = 0.0;
  for (size_t i = 0; i < count; i++) {
    parts[i] = unit(rng);
    sum += parts[i];
  }
  for (size_t i = 0; i < count; i++)
    parts[i] = parts[i] / sum * total;
  return parts;
}

static Graph BuildFlowGraph(std::mt19937 &rng) {
  Graph g{Graph::GraphFlags::Directed};
  std::uniform_int_distribution<int> layerCountDist(3, 6);
  std::uniform_int_distribution<int> layerSizeDist(2, 5);
  std::uniform_real_distribution<double> sourceTotal(10.0, 30.0);

  int layerCount = layerCountDist(rng);
  std::vector<std::vector<NodeID>> layers(layerCount);
  for (int l = 0; l < layerCount; l++) {
    int size = layerSizeDist(rng);
    for (int i = 0; i < size; i++)
      layers[l].push_back(g.AddNode());
  }

  std::vector<double> flow(layers[0].size());
  for (double &f : flow)
    f = sourceTotal(rng);

  for (int l = 0; l + 1 < layerCount; l++) {
    int leftCount = static_cast<int>(layers[l].size());
    int rightCount = static_cast<int>(layers[l + 1].size());
    auto edges = BuildBipartiteEdges(rng, leftCount, rightCount, 0.15);

    std::vector<std::vector<int>> outFromLeft(leftCount);
    for (auto &e : edges)
      outFromLeft[e.first].push_back(e.second);

    std::vector<double> nextFlow(rightCount, 0.0);
    for (int left = 0; left < leftCount; left++) {
      auto parts = RandomPartition(rng, outFromLeft[left].size(), flow[left]);
      for (size_t k = 0; k < outFromLeft[left].size(); k++) {
        int right = outFromLeft[left][k];
        g.AddEdge(layers[l][left], layers[l + 1][right],
                  static_cast<float>(parts[k]));
        nextFlow[right] += parts[k];
      }
    }
    flow = std::move(nextFlow);
  }

  return g;
}

int main() {
  std::mt19937 rng(std::random_device{}());
  Graph g = BuildFlowGraph(rng);

  SankeyLayoutAlgorithm<Graph>::Configuration layoutConfig;
  layoutConfig.thicknessScale = 4.0;
  layoutConfig.minThickness = 4.0;
  layoutConfig.nodeWidth = 24.0;
  SankeyLayoutAlgorithm<Graph> layout(g, &layoutConfig);
  layout.Tick();

  DecoratedGraph<Graph> &dg = layout.Decorated();
  auto *radii = dg.nodeSpace.SetPool<RadiusComponent>(MANUAL);
  for (NodeID nid : g.Nodes())
    radii->Find(g.MapToDense(nid).Raw())->radius = layoutConfig.nodeWidth / 2.0;

  auto *edgeColors = dg.edgeSpace.SetPool<ColorComponent>();
  for (EdgeID eid : g.Edges())
    edgeColors->Find(g.MapToDense(eid).Raw())->color =
        PackColor(0.3f, 0.55f, 0.85f, 0.65f);

  Renderer renderer(1280, 720, "gviz sankeyDemo", dg);
  renderer.SetLockToFit(true);

#ifdef GVIZ_DEBUG_CHARTS
  renderer.DebugConfigs().Add("Sankey", layoutConfig);
#endif

  while (renderer.Frame(dg)) {
    layout.Tick();
  }

  return 0;
}
