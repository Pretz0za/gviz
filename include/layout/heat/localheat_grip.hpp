#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/physics.hpp"

// implementation of the local temperature described in GRIP
// https://www2.cs.arizona.edu/~kobourov/grip_demo.pdf page 5.
template <GraphLike G> class LocalHeatGrip {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double initialHeat = 10.0 / 6.0;
    double heatR = 0.15;
    double heatS = 3.0;

    Configuration WithInitialHeat(double v) { initialHeat = v; return *this; }
    Configuration WithHeatR(double v) { heatR = v; return *this; }
    Configuration WithHeatS(double v) { heatS = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Initial Heat", &initialHeat, ConfigFieldType::Double, 0.0, 10.0},
          {"Heat R", &heatR, ConfigFieldType::Double, 0.0, 1.0},
          {"Heat S", &heatS, ConfigFieldType::Double, 0.0, 10.0},
      };
    }
  };

  LocalHeatGrip(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  void Tick(DenseNodeID id);

private:
  Configuration *m_config;
  DenseComponentPool<PhysicsComponent> *m_physicsPool;
  DenseComponentPool<LocalHeatComponent> *m_heatPool;
  uint8_t m_dimension;
};

#include "layout/heat/localheat_grip.tpp"
