#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/search/knearest.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/knearest.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/heat/localheat_grip.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"
#include "layout/physics/force_model/kamada_kawai.hpp"
#include "layout/types.hpp"

template <GraphLike G> class GRIPPhysicsSystem {
public:
  class Configuration : public IConfiguration {
  public:
    // refinementKNearest sizes DenseComponentPool<KNearestComponent>'s
    // per-entity stride at pool-creation time (see GRIPLayoutAlgorithm);
    // changing it after that pool exists would desync the pool's fixed
    // stride from the K used by RefreshKNearest below, corrupting memory.
    explicit Configuration(uint32_t refinementKNearest = 32)
        : refinementKNearest(refinementKNearest) {}

    const uint32_t refinementKNearest;
    typename GRIPFruchtermanReingold<G>::Configuration frConfig;
    typename GRIPKamadaKawai<G>::Configuration kkConfig;
    typename LocalHeatGrip<G>::Configuration heatConfig;

    std::vector<ConfigField> Fields() override { return {}; }
  };

  GRIPPhysicsSystem(DecoratedGraph<G> &dg, Configuration *config);

  Configuration *Config() { return m_config; }

  void RefreshKNearest(uint32_t currLayer);

  void Tick();

private:
  Configuration *m_config;
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
