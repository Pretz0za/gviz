#include "ds/vector.hpp"
#include "graph/graph.hpp"
#include "graph/subgraph.hpp"
#include "layout/algorithms/force_directed.hpp"
#include "layout/components/radius.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"
#include "layout/types.hpp"
#include "render/renderer.hpp"
#include <cstdint>
#include <string>

#ifdef GVIZ_DEBUG_CHARTS
#include "layout/components/force_atlas_heat.hpp"
#include "layout/components/physics.hpp"
#include <cmath>
#include <cstdio>
#endif

Graph BuildRectMesh(size_t length, size_t width) {
  Graph g;
  for (size_t i = 0; i < length; i++)
    for (size_t j = 0; j < width; j++)
      g.AddNode();

  for (size_t i = 0; i < length; i++) {
    for (size_t j = 0; j < width; j++) {
      size_t idx = i * width + j;
      if (j + 1 < width)
        g.AddUndirectedEdge(NodeID(idx), NodeID(i * width + (j + 1)), 1.0f);
      if (i + 1 < length)
        g.AddUndirectedEdge(NodeID(idx), NodeID((i + 1) * width + j), 1.0f);
    }
  }

  // g.AddUndirectedEdge(NodeID(0), NodeID((length - 1) * width + width - 1),
  //                     1.0f);
  // g.AddUndirectedEdge(NodeID(0), NodeID((length - 1) * width), 1.0f);
  // g.AddUndirectedEdge(NodeID(0), NodeID(width - 1), 1.0f);
  // g.AddUndirectedEdge(NodeID(width - 1), NodeID((length - 1) * width), 1.0);
  // g.AddUndirectedEdge(NodeID(width - 1),
  //                     NodeID((length - 1) * width + width - 1), 1.0);
  // g.AddUndirectedEdge(NodeID((length - 1) * width),
  //                     NodeID((length - 1) * width + width - 1), 1.0);
  //
  return g;
}

int main() {
  Graph parent = BuildRectMesh(30, 30);
  Subgraph g(parent);
  for (auto nid : parent.Nodes()) {

    // if (nid.Raw() % 7 != 1) {
    g.AddNode(nid);
    // }
  }
  g.SetResource<DimensionResource>(DimensionResource::D2);

  auto radii = g.NodeSpace().SetPool<RadiusComponent>(RadiusFunction::MANUAL);
  for (uint32_t i = 0; i < radii->Size(); i++) {
    if (i % 5 == 7) {
      radii->Data()[i].radius *= 4;
    }
  }

  ForceDirectedLayoutAlgorithm<Subgraph, VanillaFruchtermanReingold<Subgraph>>
      forceDirected{g};

  Renderer renderer(1280, 720, "gviz renderDemo", g);

#ifdef GVIZ_DEBUG_CHARTS
  renderer.DebugCharts().With<PhysicsComponent>(
      "disp", [](const std::vector<PhysicsComponent> &frame) {
        double sum = 0.0;
        double sumX = 0.0, sumY = 0.0;
        for (const PhysicsComponent &p : frame) {
          sum += std::sqrt(p.disp[0] * p.disp[0] + p.disp[1] * p.disp[1] +
                           p.disp[2] * p.disp[2]);
          sumX += p.disp[0];
          sumY += p.disp[1];
        }
        double avg = frame.empty() ? 0.0 : sum / frame.size();
        return std::vector<ChartDataPoint>{
            {"avg magnitude", avg}, {"sum X", sumX}, {"sum Y", sumY}};
      });

  renderer.DebugCharts().With<ForceAtlasHeatComponent>(
      "heat", [](const std::vector<ForceAtlasHeatComponent> &frame) {
        double heatSum = 0.0;
        double forceL2Sum = 0.0;
        for (const ForceAtlasHeatComponent &h : frame) {
          heatSum += h.swinging;
          forceL2Sum += L2Norm(h.oldForce, 3);
        }
        double heatAvg = frame.empty() ? 0.0 : heatSum / frame.size();
        double forceL2Avg = frame.empty() ? 0.0 : forceL2Sum / frame.size();

        return std::vector<ChartDataPoint>{{"avg swinging", heatAvg},
                                           {"avg forceL2", forceL2Avg}};
      });
#endif

  renderer.SetLockToFit(true);

  while (renderer.Frame(g)) {
    for (size_t i = 0; i < 1; i++)
      forceDirected.Tick();
  }

  return 0;
}
