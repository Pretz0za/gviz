#include "graph/graph.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/randomizer/randomizer.hpp"
#include "layout/types.hpp"
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
        g.AddEdge(NodeID(idx), NodeID(i * width + (j + 1)), 1.0);

      // down neighbor
      if (i + 1 < length)
        g.AddEdge(NodeID(idx), NodeID((i + 1) * width + j), 1.0);
    }
  }

  return g;
}

void InitializePositionComponents(Graph &g) {
  auto &admin = g.Ecs();
  auto &pool = admin.GetPool<PositionComponent>(g.NodeSpace());
  for (NodeID nid : g.Nodes()) {
	  PositionComponent &c = *pool.Find(nid.Raw());
	  c.pos[0] = 0;
	  c.pos[1] = 0;
	  c.pos[2] = 0;
  }
}

void PrintPositions(Graph &g) {
  auto &admin = g.Ecs();
  auto &pool = admin.GetPool<PositionComponent>(g.NodeSpace());
  for (NodeID nid : g.Nodes()) {
    PositionComponent *c = pool.Find(nid.Raw());
    printf("node %u: (%f, %f)\n", nid.Raw(), c->pos[0], c->pos[1]);
  }
}

int main() {

  Graph g = BuildRectMesh(2, 2);
  auto &admin = g.Ecs();

  InitializePositionComponents(g);
  admin.SetResource<DimensionResource>(DimensionResource::D2);
  PositionRandomizerSystem Randomizer{g};

  printf("initial positions:\n");
  PrintPositions(g);

  for (int i = 0; i < 3; i++) {
    Randomizer.Tick();
    printf("after tick %d:\n", i + 1);
    PrintPositions(g);
  }

  return 0;
}
