#pragma once

#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/force_directed.hpp"
#include "layout/components/force_atlas_heat.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include <memory>

template <GraphLike G, ForceModel F>
ForceDirectedLayoutAlgorithm<G, F>::ForceDirectedLayoutAlgorithm(G &graph)
    : m_graph(&graph) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);

  // pools the force model, gravity system, randomizer, and heat control all
  // rely on
  graph.NodeSpace().template SetPool<PositionComponent>(m_dimension);
  graph.NodeSpace().template SetPool<PhysicsComponent>();
  graph.NodeSpace().template SetPool<ForceAtlasHeatComponent>();

  m_forceModel = std::make_unique<F>(graph);
  m_gravity = std::make_unique<GravityForceSystem<G>>(graph);
  m_randomizer = std::make_unique<PositionRandomized<G>>(graph);
  m_heat = std::make_unique<ForceAtlasHeat<G>>(graph);

  m_randomizer->PlaceAll();
}

template <GraphLike G, ForceModel F>
void ForceDirectedLayoutAlgorithm<G, F>::Tick() {
  uint32_t size = m_graph->Size();
  DenseComponentPool<PhysicsComponent> *physics =
      m_graph->NodeSpace().template GetPool<PhysicsComponent>();
  DenseComponentPool<PositionComponent> *positions =
      m_graph->NodeSpace().template GetPool<PositionComponent>();

  for (uint32_t i = 0; i < size; i++) {
    ZeroOut(physics->Find(i)->disp, m_dimension);
  }

  for (uint32_t i = 0; i < size; i++) {
    // Repulsive Forces
    for (uint32_t j = 0; j < size; j++) {
      if (i == j)
        continue;
      m_forceModel->RepulsiveTick(DenseNodeID{i}, DenseNodeID{j});
    }

    // Attractive Forces
    NodeID nid = m_graph->MapToSparse(DenseNodeID{i});
    for (AdjEntry adj : m_graph->OutNeighbors(nid)) {
      DenseNodeID nbrDenseID = m_graph->MapToDense(adj.other);
      m_forceModel->AttractiveTick(DenseNodeID{i}, nbrDenseID);
    }
  }

  // observed before gravity so gravity doesn't skew the swinging/traction
  // measurement
  m_heat->BeginTick();
  for (uint32_t i = 0; i < size; i++) {
    m_heat->Observe(DenseNodeID{i}, physics->Find(i)->disp);
  }
  m_heat->UpdateGlobalSpeed(size);

  // m_gravity->Tick();

  for (uint32_t i = 0; i < size; i++) {
    double *disp = physics->Find(i)->disp;
    Scale(disp, m_heat->SpeedFactor(DenseNodeID{i}), m_dimension);
    Vecaxpy(1.0, disp, positions->Find(i)->pos, m_dimension);
  }
}
