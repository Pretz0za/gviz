#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/search/knearest.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/knearest.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/heat/localheat_grip.hpp"
#include "layout/physics/fruchterman_reingold.hpp"
#include "layout/physics/kamada_kawai.hpp"
#include "layout/types.hpp"

template <GraphLike G> class GRIPPhysicsSystem {
public:
  GRIPPhysicsSystem(G &graph);

  void RefreshKNearest(uint32_t currLayer);

  void Tick();

private:
  G *m_graph;
  GRIPKamadaKawai<G> m_kkForces;
  GRIPFruchtermanReingold<G> m_frForces;
  KNearestSearch<G> m_knnSystem;
  LocalHeatGrip<G> m_heatsystem;
  NestedFiltrationResult *m_filtration;
  VisibleNodesResource *m_visible;
  DenseComponentPool<KNearestComponent> *m_knearest;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<PhysicsComponent> *m_physics;
  DenseComponentPool<LocalHeatComponent> *m_heat;
  uint32_t m_currLayer = -1;
  uint8_t m_dimension = 0;
};

#include "layout/physics/grip.tpp"
