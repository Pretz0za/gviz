#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "graph/types.hpp"
#include "layout/components/force_atlas_heat.hpp"

// Global adaptive-speed control from Jacomy et al., "ForceAtlas2, a
// Continuous Graph Layout Algorithm for Handy Network Visualization
// Designed for the Gephi Software" (PLOS ONE, 2014), section "Speed
// regulation". Where LocalHeatGrip cools each node off its own force
// history, this tracks one global speed from how much the whole graph is
// swinging (oscillating) vs pulling steadily (traction), then damps each
// node by its own swinging.
template <GraphLike G> class ForceAtlasHeat {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double jitterTolerance = 1.0;
    double speedEfficiencyMin = 0.05;
    double speedEfficiencyMax = 1.0;
    double speedMaxRise = 0.5;

    Configuration WithJitterTolerance(double v) { jitterTolerance = v; return *this; }
    Configuration WithSpeedEfficiencyMin(double v) { speedEfficiencyMin = v; return *this; }
    Configuration WithSpeedEfficiencyMax(double v) { speedEfficiencyMax = v; return *this; }
    Configuration WithSpeedMaxRise(double v) { speedMaxRise = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Jitter Tolerance", &jitterTolerance, ConfigFieldType::Double, 0.0, 10.0},
          {"Speed Efficiency Min", &speedEfficiencyMin, ConfigFieldType::Double, 0.0, 1.0},
          {"Speed Efficiency Max", &speedEfficiencyMax, ConfigFieldType::Double, 0.0, 1.0},
          {"Speed Max Rise", &speedMaxRise, ConfigFieldType::Double, 0.0, 2.0},
      };
    }
  };

  ForceAtlasHeat(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  void BeginTick();
  void Observe(DenseNodeID id, const double *force);
  void UpdateGlobalSpeed(uint32_t nodeCount);
  double SpeedFactor(DenseNodeID id) const;

private:
  Configuration *m_config;
  DenseComponentPool<ForceAtlasHeatComponent> *m_heatPool;
  uint8_t m_dimension;

  double m_totalSwinging = 0.0;
  double m_totalTraction = 0.0;
  double m_globalSpeed = 1.0;
  double m_speedEfficiency = 1.0;
};

#include "layout/heat/force_atlas.tpp"
