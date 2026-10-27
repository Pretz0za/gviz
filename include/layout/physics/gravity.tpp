#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/physics/gravity.hpp"
#include "layout/types.hpp"

template <GraphLike G>
GravityForceSystem<G>::GravityForceSystem(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)), m_graph(dg.graph),
      m_physics(dg.nodeSpace.template GetPool<PhysicsComponent>()),
      m_positions(dg.nodeSpace.template GetPool<PositionComponent>()),
      m_dimension(dg.dimension) {}

template <GraphLike G> void GravityForceSystem<G>::Tick() {
  for (NodeID nid : m_graph->Nodes()) {
    uint32_t degree = m_graph->Degree(nid);
    DenseNodeID denseID = m_graph->MapToDense(nid);
    double *pos = m_positions->Find(denseID.Raw())->pos;

    double dist = L2Norm(pos, m_dimension);
    if (IsZero(dist))
      continue;

    Vecaxpy(-1 * m_config->magnitude * static_cast<double>(degree + 1) / dist, pos,
            m_physics->Find(denseID.Raw())->disp, m_dimension);
  }
}
