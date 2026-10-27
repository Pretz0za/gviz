#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/decorated_graph.hpp"
#include "layout/components/position.hpp"
#include "layout/types.hpp"
#include <cstdint>
#include <random>
#include <sys/types.h>

template <GraphLike G> class PositionRandomized {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    uint32_t boundingBoxWidth = 1000;

    Configuration WithBoundingBoxWidth(uint32_t v) { boundingBoxWidth = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Bounding Box Width", &boundingBoxWidth, ConfigFieldType::UInt32, 0.0, 1000000.0},
      };
    }
  };

  PositionRandomized(DecoratedGraph<G> &dg, Configuration *config);
  PositionRandomized(DecoratedGraph<G> &dg, uint32_t seed, Configuration *config);
  ~PositionRandomized() = default;

  Configuration *Config() { return m_config; }

  void SetSeed(uint32_t seed);

  void PlaceAll();

private:
  Configuration *m_config;
  std::mt19937 m_rng;
  std::uniform_real_distribution<double> m_dist;
  uint8_t m_dimension;
  DenseComponentPool<PositionComponent> *m_positionPool;
};

#include "layout/placement/randomized.tpp"
