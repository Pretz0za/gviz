#pragma once

#include "ecs/exceptions.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/filtration/mis_filtration.hpp"

template <GraphLike G>
GRIPLayoutAlgorithm<G>::GRIPLayoutAlgorithm(G &graph)
    : m_graph(&graph), m_placed(m_graph->Size(), 0),
      m_filtrationOutput(nullptr), m_filtrationSystem(graph) {
  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = *dim;

  BFSScratch *scratch = m_graph->template GetResource<BFSScratch>();
  if (scratch == nullptr) {
    m_graph->template SetResource<BFSScratch>(m_graph->Size());
  }
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::RunFiltration() {
	m_filtrationSystem.Tick();
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::Tick() {
  if (m_filtrationOutput->m_layerCount == 0xFFFFFFFF) {
    // filtration not run
    throw UinitializedResourceException<NestedFiltrationResult>();
  }

  // TODO: placement and refinement
}
