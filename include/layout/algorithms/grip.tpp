#pragma once

#include "ecs/components.hpp"
#include "ecs/exceptions.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/components/position.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/placement/helpers.hpp"

template <GraphLike G>
GRIPLayoutAlgorithm<G>::GRIPLayoutAlgorithm(G &graph)
    : m_graph(&graph),
      m_visible(
          &graph.template SetResource<VisibleNodesResource>(graph.Size(), 0)),
      m_placementSystem(graph),
      m_filtrationOutput(&graph.template SetResource<NestedFiltrationResult>()),
      m_filtrationSystem(graph) {

  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);

  BFSScratch *scratch =
      &m_graph->template SetResource<BFSScratch>(m_graph->Size());
  if (scratch == nullptr) {
    throw MissingResourceException<BFSScratch>();
  }

  m_filtrationOutput = m_graph->template GetResource<NestedFiltrationResult>();
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::RunFiltration() {
  m_filtrationSystem.Tick();
  m_currLayer =
      m_filtrationOutput->m_layerCount; // signals that we havent begun
}

template <GraphLike G> uint32_t GRIPLayoutAlgorithm<G>::TransitionState() {
  if (m_currLayer == 0)
    return 0;
  placeLayer();
  // TODO: add updateknns to force system here
  return m_currLayer;
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::Tick() {
  if (m_filtrationOutput->m_layerCount == 0xFFFFFFFF) {
    // filtration not run
    throw UinitializedResourceException<NestedFiltrationResult>();
  }

  // TODO: refinement
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::placeLayer() {
  auto filtration = m_filtrationOutput;
  m_currLayer--;

  if (m_currLayer == filtration->m_layerCount - 1) {
    std::vector<double> simplex(m_dimension * (m_dimension + 1));
    RegularSimplexPoints(m_dimension, 100000.0, simplex.data());

    DenseComponentPool<PositionComponent> &positions =
        m_graph->NodeSpace().template GetPool<PositionComponent>();

    PositionComponent simplexPositions[filtration->m_borders[m_currLayer]];
    FromDenseCoordinateArray(simplex, simplexPositions, m_dimension,
                             filtration->m_borders[m_currLayer]);

    for (uint32_t i = 0; i < filtration->m_borders[m_currLayer]; i++) {
      positions.Set(filtration->m_filtration[i].Raw(), simplexPositions[i]);
    }
    return;
  }

  for (uint32_t i = filtration->m_borders[m_currLayer + 1];
       i < filtration->m_borders[m_currLayer]; i++) {
    m_placementSystem.PlaceBetweenKNearest(
        m_graph->MapToSparse(filtration->m_filtration[i]), 10, *m_visible);
  }
  for (uint32_t i = filtration->m_borders[m_currLayer + 1];
       i < filtration->m_borders[m_currLayer]; i++) {
    m_visible->Set(filtration->m_filtration[i]);
  }
}
