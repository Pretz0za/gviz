#pragma once

#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/physics.hpp"
#include "layout/heat/localheat_grip.hpp"
#include "layout/types.hpp"

template <GraphLike G>
LocalHeatGrip<G>::LocalHeatGrip(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_heatPool(dg.nodeSpace.template GetPool<LocalHeatComponent>()),
      m_physicsPool(dg.nodeSpace.template GetPool<PhysicsComponent>()),
      m_dimension(dg.dimension) {}

template <GraphLike G> void LocalHeatGrip<G>::Tick(DenseNodeID id) {
  LocalHeatComponent *heat = &m_heatPool->Data()[id.Raw()];
  PhysicsComponent *physics = &m_physicsPool->Data()[id.Raw()];

  if (IsNan(physics->oldDisp, m_dimension)) {
    // no displacement history yet -- nothing to compare against.
    heat->heat = m_config->initialHeat;
	heat->oldCos = 0.0;
    return;
  }

  double nrm = L2Norm(physics->disp, m_dimension);
  double oldNrm = L2Norm(physics->oldDisp, m_dimension);

  if (!IsZero(nrm) && !IsZero(oldNrm)) {

    double cos = DotProduct(physics->disp, physics->oldDisp, m_dimension) /
                 (nrm * oldNrm);

    if (cos > 0 && heat->oldCos > 0)
      heat->heat *= (1 + cos * m_config->heatR * m_config->heatS);
    else
      heat->heat *= (1 + cos * m_config->heatR);

    heat->oldCos = cos;
  }
}
