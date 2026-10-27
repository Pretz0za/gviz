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
GRIPLayoutAlgorithm<G>::GRIPLayoutAlgorithm(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_dimension(m_config->dimension), m_dg(graph) {

  m_dg.dimension = m_dimension;
  m_visible = &m_dg.template SetResource<VisibleNodesResource>(graph.Size(), 0);
  m_filtrationOutput = &m_dg.template SetResource<NestedFiltrationResult>();
  m_dg.template SetResource<BFSScratch>(graph.Size());

  // pools grip's systems use
  m_dg.nodeSpace.template SetPool<PositionComponent>(m_dimension);
  m_dg.nodeSpace.template SetPool<KNearestComponent>(
      m_config->physicsConfig.refinementKNearest);
  m_dg.nodeSpace.template SetPool<LocalHeatComponent>();
  m_dg.nodeSpace.template SetPool<PhysicsComponent>(); // TODO: this should be
                                                       // m_dimension based

  // systems grip uses
  m_placementSystem = std::make_unique<PositionBarrycentric<G>>(m_dg);
  m_filtrationSystem = std::make_unique<MisFiltrationSystem<G>>(m_dg);
  m_physicsSystem =
      std::make_unique<GRIPPhysicsSystem<G>>(m_dg, &m_config->physicsConfig);

#ifdef GVIZ_DEBUG_CHARTS
  m_chart = m_dg.template SetResource<ChartRecorderResource>(m_dg.nodeSpace)
                .recorder.get();
#endif
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

#ifdef GVIZ_DEBUG_CHARTS
  m_chart->PushFrame<PhysicsComponent>();
#endif
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::placeLayer() {
  auto filtration = m_filtrationOutput;
  m_currLayer--;

  if (m_currLayer == filtration->m_layerCount - 1) {
    std::vector<double> simplex(m_dimension * (m_dimension + 1));
    RegularSimplexPoints(m_dimension, m_config->simplexExtent, simplex.data());

    DenseComponentPool<PositionComponent> *positions =
        m_dg.nodeSpace.template GetPool<PositionComponent>();

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
        m_dg.graph->MapToSparse(filtration->m_filtration[i]),
        m_config->placementKNearest, *m_visible);
  }
  for (uint32_t i = filtration->m_borders[m_currLayer + 1];
       i < filtration->m_borders[m_currLayer]; i++) {
    m_visible->Set(filtration->m_filtration[i]);
  }
}
