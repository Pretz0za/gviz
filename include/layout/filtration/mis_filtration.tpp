#pragma once

#include "concept/graphLike.hpp"
#include "ds/bfs_scratch.hpp"
#include "ds/bitset.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "ecs/exceptions.hpp"
#include <cstdint>

template <GraphLike G>
MisFiltrationSystem<G>::MisFiltrationSystem(G &graph)
    : m_graph(&graph), m_scratch(nullptr) {
  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = *dim;

  m_scratch = m_graph->template GetResource<BFSScratch>();
  if (m_scratch == nullptr)
    throw MissingResourceException<BFSScratch>();

  m_output = m_graph->template GetResource<NestedFiltrationResult>();
  if (m_output == nullptr)
    m_output = &m_graph->template SetResource<NestedFiltrationResult>();

  m_output->m_layerCount = 0xFFFFFFFF;
}

template <GraphLike G> void MisFiltrationSystem<G>::Tick() {
  m_output->m_filtration.resize(m_graph->Size());
  m_output->m_borders.clear();
  m_output->m_layerCount = 0;
  BuildFiltration();
}

template <GraphLike G> void MisFiltrationSystem<G>::BuildFiltration() {
  size_t nvertices = m_graph->Size();

  // layer 0, full graph
  BitSet currLayer{m_graph->Size(), 1};
  m_output->m_borders.push_back(m_graph->Size());
  m_output->m_layerCount = 1;

  while (BuildNextLayer(currLayer)) {
  }

  // write the final layer
  size_t k = 0;
  for (size_t vtx : currLayer)
    m_output->m_filtration[k++] = vtx;

  // ensures last layer has enough for a simplex
  m_output->m_borders[m_output->m_layerCount - 1] =
      static_cast<uint8_t>(m_dimension) + 1;
}

template <GraphLike G>
bool MisFiltrationSystem<G>::BuildNextLayer(BitSet &lastLayer) {
  uint32_t i = m_output->m_layerCount;
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

    MarkVerticesWithinRadius(m_graph->MapToSparse(DenseNodeID(node)), radius,
                             marked);
  }

  size_t writePos = m_output->m_borders[i - 1] - 1;
  for (size_t curr : lastLayer) {
    if (!newLayer.Test(curr)) {
      m_output->m_filtration[writePos--] = curr;
    }
  }

  bool cont = count > static_cast<uint8_t>(m_dimension) + 1;

  m_output->m_borders.push_back(count);
  m_output->m_layerCount++;
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

      DenseNodeID nbrCompact = m_graph->MapToDense(adj.other);

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
