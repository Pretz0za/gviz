#include "graph/graph.hpp"
#include "graph/subgraph.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/components/position.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/types.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>

Graph BuildRectMesh(size_t length, size_t width) {
  Graph g;
  for (size_t i = 0; i < length; i++)
    for (size_t j = 0; j < width; j++)
      g.AddNode();

  for (size_t i = 0; i < length; i++) {
    for (size_t j = 0; j < width; j++) {
      size_t idx = i * width + j;

      // right neighbor
      if (j + 1 < width)
        g.AddUndirectedEdge(NodeID(idx), NodeID(i * width + (j + 1)), 1.0);

      // down neighbor
      if (i + 1 < length)
        g.AddUndirectedEdge(NodeID(idx), NodeID((i + 1) * width + j), 1.0);
    }
  }

  return g;
}

void InitializePositionComponents(Graph &g) {
  auto *pool = g.NodeSpace().GetPool<PositionComponent>();
  for (NodeID nid : g.Nodes()) {
    PositionComponent *c = pool->Find(nid.Raw());
    c->pos[0] = 0;
    c->pos[1] = 0;
    c->pos[2] = 0;
  }
}

void PrintPositions(Graph &g) {
  auto *pool = g.NodeSpace().GetPool<PositionComponent>();
  for (NodeID nid : g.Nodes()) {
    PositionComponent *c = pool->Find(nid.Raw());
    printf("node %u: (%f, %f)\n", nid.Raw(), c->pos[0], c->pos[1]);
  }
}

void PrintGripLayerPositions(Graph *g, NestedFiltrationResult *filtration,
                             uint8_t layer) {
  auto *pool = g->NodeSpace().GetPool<PositionComponent>();
  for (uint32_t i = 0; i < filtration->m_borders[layer]; i++) {
    DenseNodeID nid = filtration->m_filtration[i];
    PositionComponent *c = pool->Find(nid.Raw());
    printf("node %u: (%f, %f)\n", nid.Raw(), c->pos[0], c->pos[1]);
  }
}

// Prints "(x1,y1),(x2,y2),..." for every node's position -- paste directly
// into a Desmos expression to plot the list as points.
void PrintPositionsDesmos(Graph &g) {
  auto *pool = g.NodeSpace().GetPool<PositionComponent>();
  bool first = true;
  for (NodeID nid : g.Nodes()) {
    PositionComponent *c = pool->Find(nid.Raw());
    if (!first)
      printf(",");
    printf("(%f,%f)", c->pos[0], c->pos[1]);
    first = false;
  }
  printf("\n");
}

int main() {

  Graph g = BuildRectMesh(10, 100);
  Subgraph sg{g};

  for (NodeID nid : g.Nodes()) {
    sg.AddNode(nid);
  }

  // InitializePositionComponents(g);
  sg.SetResource<DimensionResource>(DimensionResource::D2);
  g.SetResource<DimensionResource>(DimensionResource::D2);
  // PositionRandomizerSystem Randomizer{g};

  // printf("initial positions:\n");
  // PrintPositions(g);
  //
  // for (int i = 0; i < 3; i++) {
  //   Randomizer.Tick();
  //   printf("after tick %d:\n", i + 1);
  //   PrintPositions(g);
  // }

  GRIPLayoutAlgorithm grip{g};
  grip.RunFiltration();

  NestedFiltrationResult result = *g.GetResource<NestedFiltrationResult>();

  uint8_t layer = grip.TransitionState();
  layer = grip.TransitionState();

  for (uint32_t i = 0; i < result.m_layerCount; i++) {
    grip.TransitionState();
    for (uint32_t j = 0; j < 100; j++) {
      grip.Tick();
    }
  }

  // for (uint32_t i = 0; i < 100; i++) {
  //   printf("\n\n\nLAYER %u POSITIONS AT TICK %u\n", layer, i);
  //   PrintGripLayerPositions(&g, &result, layer);
  //   grip.Tick();
  // }

  PrintPositionsDesmos(g);

  // MisFiltrationSystem filtration(sg);
  // filtration.Tick();

  // assert(sg.GetResource<NestedFiltrationResult>() != nullptr);

  printf("layer count: %d\n", result.m_layerCount);

  printf("last 10 in filtration: ");
  for (size_t i = 0; i < 10; i++) {
    size_t idx = g.Size() - 1 - i;
    printf("%d, ", result.m_filtration[idx].Raw());
  }

  printf("\nborders: ");

  for (size_t i = 0; i < result.m_layerCount; i++) {
    printf("%d, ", result.m_borders[i]);
  }

  printf("\n");

  return 0;
}
