#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"

template <GraphLike G> class GravityForceSystem {

public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double magnitude = 0.0;

    Configuration WithMagnitude(double v) { magnitude = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Magnitude", &magnitude, ConfigFieldType::Double, 0.0, 10.0},
      };
    }
  };

  GravityForceSystem(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  inline void Tick();

private:
  Configuration *m_config;
  G *m_graph;
  DenseComponentPool<PhysicsComponent> *m_physics;
  DenseComponentPool<PositionComponent> *m_positions;
  uint8_t m_dimension;
};
#include "layout/physics/gravity.tpp"
