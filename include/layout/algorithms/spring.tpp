#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/spring.hpp"
#include "layout/types.hpp"

template <GraphLike G>
SpringLayoutAlgorithm<G>::SpringLayoutAlgorithm(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)), m_graph(&graph) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);

  m_positions =
      graph.NodeSpace().template SetPool<PositionComponent>(m_dimension);
  m_velocity = graph.NodeSpace().template SetPool<VelocityComponent>();
  m_physics = graph.NodeSpace().template SetPool<PhysicsComponent>();
  m_mass = graph.NodeSpace().template SetPool<MassComponent>();
  m_springs = graph.EdgeSpace().template SetPool<SpringComponent>();

  for (SpringComponent &spring : m_springs->Data()) {
    spring.stiffness = m_config->stiffness;
    spring.restLength = m_config->restLength;
  }

  m_fixed = &graph.template SetResource<FixedNodesResource>(graph.Size(), 0);
}

template <GraphLike G>
void SpringLayoutAlgorithm<G>::Fix(NodeID node) {
  DenseNodeID dense = m_graph->MapToDense(node);
  m_fixed->Set(dense);
  ZeroOut(m_velocity->Find(dense.Raw())->vel, m_dimension);
}

template <GraphLike G>
void SpringLayoutAlgorithm<G>::Unfix(NodeID node) {
  m_fixed->Clear(m_graph->MapToDense(node));
}

template <GraphLike G>
bool SpringLayoutAlgorithm<G>::IsFixed(NodeID node) const {
  return m_fixed->Test(m_graph->MapToDense(node));
}

template <GraphLike G>
void SpringLayoutAlgorithm<G>::SetPosition(NodeID node, const double *pos) {
  DenseNodeID dense = m_graph->MapToDense(node);
  Copy(pos, m_positions->Find(dense.Raw())->pos, m_dimension);
  ZeroOut(m_velocity->Find(dense.Raw())->vel, m_dimension);
}

template <GraphLike G>
void SpringLayoutAlgorithm<G>::Tick() {
  uint32_t size = m_graph->Size();

  for (uint32_t i = 0; i < size; i++)
    ZeroOut(m_physics->Find(i)->disp, m_dimension);

  double diff[3];
  for (NodeID nid : m_graph->Nodes()) {
    DenseNodeID vDense = m_graph->MapToDense(nid);
    double *vPos = m_positions->Find(vDense.Raw())->pos;
    double *vForce = m_physics->Find(vDense.Raw())->disp;

    for (AdjEntry adj : m_graph->OutNeighbors(nid)) {
      DenseNodeID uDense = m_graph->MapToDense(adj.other);
      DenseEdgeID edgeDense = m_graph->MapToDense(adj.edge);
      SpringComponent *spring = m_springs->Find(edgeDense.Raw());
      double *uPos = m_positions->Find(uDense.Raw())->pos;

      Subtract(uPos, vPos, diff, m_dimension);
      double length = L2Norm(diff, m_dimension);
      if (IsZero(length))
        continue;

      Scale(diff, spring->stiffness * (length - spring->restLength) / length,
            m_dimension);
      Vecaxpy(1.0, diff, vForce, m_dimension);
    }
  }

  for (uint32_t i = 0; i < size; i++) {
    DenseNodeID dense{i};
    if (m_fixed->Test(dense)) {
      ZeroOut(m_velocity->Find(i)->vel, m_dimension);
      continue;
    }

    double *force = m_physics->Find(i)->disp;
    double *vel = m_velocity->Find(i)->vel;
    double mass = m_mass->Find(i)->mass;

    Vecaxpy(-m_config->damping, vel, force, m_dimension);
    Vecaxpy(m_config->timeStep / mass, force, vel, m_dimension);
    Vecaxpy(m_config->timeStep, vel, m_positions->Find(i)->pos, m_dimension);
  }
}
