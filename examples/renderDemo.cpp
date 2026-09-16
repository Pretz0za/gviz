#include "graph/graph.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/filtration/mis_filtration.hpp"
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

  return g;
}

int main() {
  Graph g = BuildRectMesh(10, 10);
  g.SetResource<DimensionResource>(DimensionResource::D2);

  GRIPLayoutAlgorithm grip{g};
  grip.RunFiltration();

  NestedFiltrationResult result = *g.GetResource<NestedFiltrationResult>();
  for (uint32_t i = 0; i < result.m_layerCount; i++) {
    grip.TransitionState();
    for (uint32_t j = 0; j < 100; j++)
      grip.Tick();
  }

  Renderer renderer(1280, 720, "gviz renderDemo");
  while (renderer.Frame(g)) {
  }

  return 0;
}
