#pragma once

#include "ds/quadtree.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/force_directed.hpp"
#include "layout/algorithms/types.hpp"
#include "layout/components/force_atlas_heat.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/concept/force_model.hpp"
#include <cmath>
#include <cstdio>
#include <memory>

template <GraphLike G, ForceModel F>
ForceDirectedLayoutAlgorithm<G, F>::ForceDirectedLayoutAlgorithm(G &graph)
    : m_graph(&graph), m_positions(nullptr), m_mass(nullptr) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);

  m_quadtree = &graph.template SetResource<QuadTreeResource>();

  // pools the force model, gravity system, randomizer, and heat control all
  // rely on
  m_mass = graph.NodeSpace().template SetPool<MassComponent>();
  m_positions =
      graph.NodeSpace().template SetPool<PositionComponent>(m_dimension);
  graph.NodeSpace().template SetPool<PhysicsComponent>();
  graph.NodeSpace().template SetPool<ForceAtlasHeatComponent>();

  m_forceModel = std::make_unique<F>(graph);
  m_gravity = std::make_unique<GravityForceSystem<G>>(graph);
  m_randomizer = std::make_unique<PositionRandomized<G>>(graph);
  m_heat = std::make_unique<ForceAtlasHeat<G>>(graph);

#ifdef GVIZ_DEBUG_CHARTS
  m_chart = graph.template SetResource<ChartRecorderResource>(graph.NodeSpace())
                .recorder.get();
#endif

  double boxExtent = 0.5 * std::sqrt(static_cast<double>(graph.Size())) *
                      m_forceModel->EdgeLength();
  if (boxExtent > 0.0)
    m_randomizer->SetBoundingBox(static_cast<uint32_t>(boxExtent));

  m_randomizer->PlaceAll();
}

template <GraphLike G, ForceModel F>
void ForceDirectedLayoutAlgorithm<G, F>::RebuildQuadTree() {
  SpacialIndex::AABB bbox = SpacialIndex::Helpers::GetBoundingBox(m_graph);
  m_quadtree->root->Reset(bbox);

  auto &positions = m_positions->Data();
  for (uint32_t i = 0; i < positions.Size(); i++) {
    bool inserted = m_quadtree->root->Insert(
        DenseNodeID{i}, {positions[i].pos[0], positions[i].pos[1]},
        m_mass->Find(i)->mass);
    if (!inserted) {
      fprintf(stderr, "[quadtree] Insert failed for node %u at (%.6f, %.6f)\n",
              i, positions[i].pos[0], positions[i].pos[1]);
    }
  }
}

template <GraphLike G, ForceModel F>
void ForceDirectedLayoutAlgorithm<G, F>::RepulsiveTick(
    SpacialIndex::QuadTree<DenseNodeID, 1> *node, DenseNodeID v) {
  if (!node || node->Mass() == 0.0)
    return;

  if (node->IsLeaf()) {
    // only one vertex in the quadtree node, by construction
    DenseNodeID DenseID = node->DataAt(0);
    if (DenseID == v)
      return;
    const auto *uPos = m_positions->Find(DenseID.Raw());
    m_forceModel->RepulsiveTick(v, uPos->pos, node->Mass());
    return;
  }

  const auto *vPos = m_positions->Find(v.Raw());

  SpacialIndex::Point com = node->CenterOfMass();
  double dx = com[0] - vPos->pos[0];
  double dy = com[1] - vPos->pos[1];
  double dist = std::sqrt(dx * dx + dy * dy);
  double ratio = (2.0 * node->HalfLength()) / dist;

  // TODO: theta moved to config
  if (ratio < 1.0) {
    m_forceModel->RepulsiveTick(v, com.data(), node->Mass());
    return;
  }

  for (size_t q = 0; q < SpacialIndex::QuadTreeQuadrant::COUNT; q++)
    this->RepulsiveTick(
        node->Quadrant(static_cast<SpacialIndex::QuadTreeQuadrant>(q)), v);
}

template <GraphLike G, ForceModel F>
void ForceDirectedLayoutAlgorithm<G, F>::Tick() {
  RebuildQuadTree();
  uint32_t size = m_graph->Size();
  DenseComponentPool<PhysicsComponent> *physics =
      m_graph->NodeSpace().template GetPool<PhysicsComponent>();
  DenseComponentPool<PositionComponent> *positions =
      m_graph->NodeSpace().template GetPool<PositionComponent>();

  for (uint32_t i = 0; i < size; i++) {
    ZeroOut(physics->Find(i)->disp, m_dimension);
  }

  for (uint32_t i = 0; i < size; i++) {
    RepulsiveTick(m_quadtree->root.get(), DenseNodeID{i});

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

#ifdef GVIZ_DEBUG_CHARTS
  m_chart->PushFrame<PhysicsComponent>();
  m_chart->PushFrame<ForceAtlasHeatComponent>();
#endif
}
