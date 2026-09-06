#pragma once

#include "concept/graphLike.hpp"
#include "ds/bfs_scratch.hpp"
#include "ds/bitset.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/randomizer/randomizer.tpp"
#include <cstdint>

template <GraphLike G>
MisFiltrationSystem<G>::MisFiltrationSystem(G &graph)
    : m_graph(&graph), m_scratch(nullptr) {
  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = *dim;

  BFSScratch *scratch = m_graph->template GetResource<BFSScratch>();
  if (scratch == nullptr) {
    m_scratch = &m_graph->template SetResource<BFSScratch>(m_graph->Size());
  } else {
    m_scratch = scratch;
  }
}

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

  // layer 0, full graph
  BitSet currLayer{m_graph->Size(), 1};
  out.m_borders.push_back(m_graph->Size());
  out.m_layerCount = 1;

  while (BuildNextLayer(out, currLayer)) {
  }

  // write the final layer
  size_t k = 0;
  for (size_t vtx : currLayer)
    out.m_filtration[k++] = vtx;

  // while (out.m_borders[out.m_layerCount - 1] <
  // static_cast<uint8_t>(m_dimension) + 1) {
  //   if (!MigrateOneToFinalLayer(out.m_layerCount + 1))
  //     break;
  // }
}

template <GraphLike G>
bool MisFiltrationSystem<G>::BuildNextLayer(NestedFiltrationResult &out,
                                            BitSet &lastLayer) {
  uint32_t i = out.m_layerCount;
  uint32_t count = 0;
  size_t nvertices = m_graph->Size();
  BitSet newLayer(nvertices, 0);
  BitSet marked(nvertices, 0);

  uint32_t radius = uint32_t{1} << (i - 1);

  for (size_t node : lastLayer) {
    if (marked.Test(node))
      continue;

    newLayer.Set(node);
    count++;
    // marked.Set(curr);

    MarkVerticesWithinRadius(m_graph->MapToSparse(NodeID(node)), radius, marked);
  }

  size_t writePos = out.m_borders[i - 1] - 1;
  for (size_t curr : lastLayer) {
    if (!newLayer.Test(curr)) {
      out.m_filtration[writePos--] = curr;
    }
  }

  bool cont = count > static_cast<uint8_t>(m_dimension) + 1;

  out.m_borders.push_back(count);
  out.m_layerCount++;
  // NOTE: look into this copy
  lastLayer = newLayer;
  return cont;
}

template <GraphLike G>
void MisFiltrationSystem<G>::MarkVerticesWithinRadius(NodeID source,
                                                      uint32_t radius,
                                                      BitSet &marked) {
  m_scratch->InitNew();
  m_scratch->Push(source, 0);

  while (!m_scratch->Empty()) {
    FoundNode nd = m_scratch->Pop();

    if (radius && nd.depth >= radius)
      continue;

    for (AdjEntry adj : m_graph->OutNeighbors(nd.node)) {

      NodeID nbrCompact = m_graph->MapToDense(adj.other);

      if (m_scratch->IsVisited(nbrCompact))
        continue;
      m_scratch->Visit(nbrCompact);

      uint32_t nextDepth = nd.depth + 1;
      if (nextDepth <= radius) {
        marked.Set(nbrCompact.Raw());
      }

      m_scratch->Push(adj.other, nextDepth);
    }
  }
}
