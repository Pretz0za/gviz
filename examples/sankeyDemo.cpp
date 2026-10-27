#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "layout/algorithms/sankey.hpp"
#include "layout/components/radius.hpp"
#include "render/components/color.hpp"
#include "render/renderer.hpp"
#include <cstdint>
#include <random>

static uint32_t PackColor(float r, float g, float b, float a) {
  auto c = [](float v) { return static_cast<uint32_t>(v * 255.0f) & 0xFFu; };
  return (c(r) << 24) | (c(g) << 16) | (c(b) << 8) | c(a);
}

static Graph BuildFlowGraph(std::mt19937 &rng) {
  Graph g;
  std::uniform_real_distribution<float> weight(1.0f, 6.0f);

  std::vector<NodeID> sources;
  for (int i = 0; i < 3; i++)
    sources.push_back(g.AddNode());

  std::vector<NodeID> hubs;
  for (int i = 0; i < 4; i++)
    hubs.push_back(g.AddNode());

  std::vector<NodeID> sinks;
  for (int i = 0; i < 5; i++)
    sinks.push_back(g.AddNode());

  for (NodeID s : sources)
    for (NodeID h : hubs)
      if (rng() % 2 == 0)
        g.AddEdge(s, h, weight(rng));

  for (NodeID h : hubs)
    for (NodeID t : sinks)
      if (rng() % 2 == 0)
        g.AddEdge(h, t, weight(rng));

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
