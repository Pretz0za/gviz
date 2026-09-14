#pragma once

#include "ecs/components.hpp"
#include "ecs/exceptions.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/knearest.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/physics/grip.hpp"
#include "layout/placement/barrycenter.hpp"
#include "layout/placement/helpers.hpp"
#include <memory>

template <GraphLike G>
GRIPLayoutAlgorithm<G>::GRIPLayoutAlgorithm(G &graph)
    : m_graph(&graph),
      m_visible(
          &graph.template SetResource<VisibleNodesResource>(graph.Size(), 0)),
      m_filtrationOutput(
          &graph.template SetResource<NestedFiltrationResult>()) {

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

  // pools grip's systems use
  m_graph->NodeSpace().template SetPool<PositionComponent>(m_dimension);
  m_graph->NodeSpace().template SetPool<KNearestComponent>(32); // TODO: config
  m_graph->NodeSpace().template SetPool<LocalHeatComponent>();
  m_graph->NodeSpace()
      .template SetPool<PhysicsComponent>(); // TODO: this should be m_dimension
                                             // based

  // systems grip uses
  m_placementSystem = std::make_unique<PositionBarrycentric<G>>(graph);
  m_filtrationSystem = std::make_unique<MisFiltrationSystem<G>>(graph);
  m_physicsSystem = std::make_unique<GRIPPhysicsSystem<G>>(graph);
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::RunFiltration() {
  m_filtrationSystem->Tick();
  m_currLayer =
      m_filtrationOutput->m_layerCount; // signals that we havent begun
}

template <GraphLike G> uint32_t GRIPLayoutAlgorithm<G>::TransitionState() {
  if (m_currLayer == 0)
    return 0;
  placeLayer();
  m_physicsSystem->RefreshKNearest(m_currLayer);
  // TODO: add updateknns to force system here
  return m_currLayer;
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::Tick() {

  if (m_filtrationOutput->m_layerCount == 0xFFFFFFFF) {
    // filtration not run
    throw UinitializedResourceException<NestedFiltrationResult>();
  }

  m_physicsSystem->Tick();
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::placeLayer() {
  auto filtration = m_filtrationOutput;
  m_currLayer--;

  if (m_currLayer == filtration->m_layerCount - 1) {
    std::vector<double> simplex(m_dimension * (m_dimension + 1));
    RegularSimplexPoints(m_dimension, 100000.0, simplex.data());

    DenseComponentPool<PositionComponent> *positions =
        m_graph->NodeSpace().template GetPool<PositionComponent>();

    uint32_t count = filtration->m_borders[m_currLayer];
    std::vector<double> simplexPositions(count * m_dimension);
    FromDenseCoordinateArray(simplex.data(), simplexPositions.data(),
                             m_dimension, count);

    auto &data = positions->Data();
    for (uint32_t i = 0; i < count; i++) {
      DenseNodeID id = filtration->m_filtration[i];
      for (uint8_t d = 0; d < m_dimension; d++) {
        data[id.Raw()].pos[d] = simplexPositions[i * m_dimension + d];
        m_visible->Set(id);
      }
    }

    return;
  }

  for (uint32_t i = filtration->m_borders[m_currLayer + 1];
       i < filtration->m_borders[m_currLayer]; i++) {
    m_placementSystem->PlaceBetweenKNearest(
        m_graph->MapToSparse(filtration->m_filtration[i]), 10, *m_visible);
  }
  for (uint32_t i = filtration->m_borders[m_currLayer + 1];
       i < filtration->m_borders[m_currLayer]; i++) {
    m_visible->Set(filtration->m_filtration[i]);
  }
}
