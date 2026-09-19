#include "graph/graph.hpp"
#include "graph/subgraph.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/types.hpp"
#include "render/renderer.hpp"

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
  Graph parent = BuildRectMesh(10, 10);
  Subgraph g(parent);
  for (auto nid : parent.Nodes()) {

    if (nid.Raw() % 7 != 1) {
      g.AddNode(nid);
    }
  }
  g.SetResource<DimensionResource>(DimensionResource::D2);

  auto radii = g.NodeSpace().SetPool<RadiusComponent>();
  for (uint32_t i = 0; i < radii->Size(); i++) {
    if (i % 5 == 7) {
      radii->Data()[i].radius *= 4;
    }
  }

  GRIPLayoutAlgorithm grip{g};
  grip.RunFiltration();
  uint32_t iteration = 0;
  grip.TransitionState();

  Renderer renderer(1280, 720, "gviz renderDemo", g);
  while (renderer.Frame(g)) {
    if (iteration >= 50) {
      grip.TransitionState();
      iteration = 0;
    }
    grip.Tick();
    iteration++;
  }

  return 0;
}
