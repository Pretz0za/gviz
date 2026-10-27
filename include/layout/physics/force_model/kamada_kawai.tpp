#pragma once

#include "ds/vector.hpp"
#include "layout/physics/force_model/kamada_kawai.hpp"

template <GraphLike G>
GRIPKamadaKawai<G>::GRIPKamadaKawai(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_physics(dg.nodeSpace.template GetPool<PhysicsComponent>()),
      m_distanceCalc(dg), m_dimension(dg.dimension) {}

template <GraphLike G>
void GRIPKamadaKawai<G>::Tick(DenseNodeID v, DenseNodeID u,
                              uint32_t graphDist) {
  auto &physics = m_physics->Data();
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(v, u, out);
  double spacing = m_config->targetSpacing;
  Vecaxpy((m_distanceCalc.BetweenNodes(v, u) /
           (static_cast<double>(graphDist) * spacing * spacing)) -
              1,
          out, physics[v.Raw()].disp, m_dimension);
}
