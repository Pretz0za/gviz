#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "graph/search/types.hpp"
#include "graph/types.hpp"
#include "layout/components/knearest.hpp"
#include "layout/physics/grip.hpp"
#include "layout/types.hpp"
#include <algorithm>
#include <limits>

template <GraphLike G>
GRIPPhysicsSystem<G>::GRIPPhysicsSystem(G &graph)
    : m_graph(&graph), m_knnSystem(graph), m_kkForces(graph), m_frForces(graph),
      m_knearest(&graph.NodeSpace().template GetPool<KNearestComponent<32>>()),
      m_positions(&graph.NodeSpace().template GetPool<PositionComponent()>),
      m_physics(&graph.NodeSpace().template GetPool<PhysicsComponent()>),
      m_heat(&graph.NodeSpace().template GetPool<LocalHeatComponent()>),
      m_filtration(graph.template GetResource<NestedFiltrationResult>()),
      m_visible(graph.template GetResource<VisibleNodesResource>()),
      m_heatsystem(graph) {

  if (m_filtration == nullptr)
    throw MissingResourceException<NestedFiltrationResult>();

  if (m_visible == nullptr)
    throw MissingResourceException<VisibleNodesResource>();

  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G>
void GRIPPhysicsSystem<G>::RefreshKNearest(uint32_t currLayer) {
  m_currLayer = currLayer;
  auto &data = m_knearest->Data();
  uint32_t end = m_filtration->m_borders[m_currLayer];
  for (uint32_t i = 0; i < end; i++) {
    DenseNodeID denseID = m_filtration->m_filtration[i];
    NodeID sparseID = m_graph->MapToSparse(denseID);
    std::vector<FoundNode> knn = m_knnSystem.Find(sparseID, 32, *m_visible);
    data[denseID.Raw()].size = knn.size();
    std::ranges::move(knn, data[denseID.Raw()].nearest);
  }
}

template <GraphLike G> void GRIPPhysicsSystem<G>::Tick() {
  if (m_currLayer == std::numeric_limits<uint32_t>::max())
    return;

  uint32_t end = m_filtration->m_borders[m_currLayer];
  auto &knns = m_knearest->Data();
  auto &positions = m_positions->Data();
  auto &physics = m_physics->Data();
  auto &heat = m_heat->Data();

  for (uint32_t i = 0; i < end; i++) {
    DenseNodeID denseID = m_filtration->m_filtration[i];
    if (false) // if (m_currLayer == 0)
      ;
    else {
      for (uint32_t j = 0; j < knns[denseID.Raw()].size; j++) {
        FoundNode fn = knns[denseID.Raw()].nearest[j];
        m_kkForces.Tick(denseID, m_graph->MapToDense(fn.node), fn.depth);
      }
    }
    m_heatsystem.Tick(denseID);

    Copy(physics[denseID.Raw()].disp, physics[denseID.Raw()].oldDisp,
         m_dimension);

    double nrm = L2Norm(physics[denseID.Raw()].disp, m_dimension);
    if (nrm > 0) {
      Scale(physics[denseID.Raw()].disp, heat[denseID.Raw()].heat / nrm,
            m_dimension);
    }
  }
}
