#pragma once

#include "ds/vector.hpp"
#include "layout/physics/force_model/kamada_kawai.hpp"

template <GraphLike G>
GRIPKamadaKawai<G>::GRIPKamadaKawai(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()),
      m_distanceCalc(graph) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
}

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
