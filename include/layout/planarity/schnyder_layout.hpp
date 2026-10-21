#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "layout/components/position.hpp"
#include "layout/planarity/rotation_system.hpp"
#include "layout/planarity/schnyder_wood.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

template <GraphLike G> class SchnyderLayoutAlgorithm {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double scale = 500.0;

    Configuration WithScale(double v) {
      scale = v;
      return *this;
    }

    std::vector<ConfigField> Fields() override {
      return {
          {"Scale", &scale, ConfigFieldType::Double, 1.0, 100000.0},
      };
    }
  };

  SchnyderLayoutAlgorithm(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  void Layout();
  void Tick();
  void RandomizeRoots();

private:
  void RebuildWood(NodeID s0, NodeID s1);
  void ComputeCoordinates();

  Configuration *m_config;
  G *m_graph;
  DenseComponentPool<PositionComponent> *m_positions;
  PlanarRotationSystem<G> m_rotation;
  std::unique_ptr<SchnyderWood<G>> m_wood;
  std::array<NodeID, 3> m_root{};
  uint64_t m_lastConfigHash = 0;
  bool m_hasRun = false;
  std::mt19937 m_rng{std::random_device{}()};
};

#include "layout/planarity/schnyder_layout.tpp"
