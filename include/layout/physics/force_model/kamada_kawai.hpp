#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/components/physics.hpp"
#include "layout/helpers/distance.hpp"
#include <cstdint>

class VanillaKamadaKawai {};

template <GraphLike G> class GRIPKamadaKawai {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double targetSpacing = 10.0;

    Configuration WithTargetSpacing(double v) { targetSpacing = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Target Spacing", &targetSpacing, ConfigFieldType::Double, 0.1, 200.0},
      };
    }
  };

  GRIPKamadaKawai(DecoratedGraph<G> &dg, Configuration *config);

  Configuration *Config() { return m_config; }

  void Tick(DenseNodeID v, DenseNodeID u, uint32_t graphDist);

private:
  Configuration *m_config;
  DenseComponentPool<PhysicsComponent> *m_physics;
  DistanceCalculationSystem<G> m_distanceCalc;
  uint8_t m_dimension;
};

#include "layout/physics/force_model/kamada_kawai.tpp"
