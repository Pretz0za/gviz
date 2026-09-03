#pragma once

#include "concept/graphLike.hpp"
#include "ecs/admin.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include <cstdint>

template <GraphLike G>
MisFiltrationSystem<G>::MisFiltrationSystem(G &graph) : m_graph(&graph) {}

template <GraphLike G>
void MisFiltrationSystem<G>::Tick(NestedFiltrationResult &out) {
  out.m_filtration.reserve(m_graph->Size());
  out.m_filtration.clear();
  out.m_borders.clear();
  out.m_layerCount = 0;
  BuildFiltration(out);
}

template <GraphLike G>
void MisFiltrationSystem<G>::BuildFiltration(NestedFiltrationResult &out) {
  size_t nvertices = m_graph->Size();
  Bitset curr(nvertices);

  BuildFirstLayer(out, curr);

  uint32_t i = 2;
  while (IterMISFiltration(i, curr))
    i++;

  // NOTE: why is this loop even needed?
  size_t k = 0;
  for (size_t vtx : curr)
    misFiltration_[k++] = vtx;

  while (MisBorderAt(i) < Dim() + 1) {
    if (!MigrateOneToFinalLayer(i + 1))
      break;
  }

  return i + 1;
}

template <GraphLike G>
void MisFiltrationSystem<G>::BuildFirstLayer(NestedFiltrationResult &out,
                                             Bitset &vertices) {
  size_t writePos = m_graph->Size() - 1;
  Bitset marked(m_graph->Size());
  for (NodeID v : m_graph->Nodes()) {
    uint32_t compact = m_graph->ToCompact(v);

    if (marked.Test(compact))
      continue;

    vertices.Set(compact);
    for (const AdjEntry &adj : m_graph->OutEdges(compact)) {
      uint32_t nbrCompact = m_graph->ToCompact(adj.other);
      if (!marked.Test(nbrCompact)) {
        marked.Set(m_graph->ToCompact(adj.other));
		out.m_filtration[writePos] = adj.other;
      }
    }
  }
}
