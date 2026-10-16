#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "debug/chart_recorder.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include "layout/physics/grip.hpp"
#include "layout/placement/barrycenter.hpp"
#include <memory>

// implementation of the GRIP algorithm by Gajer and Kobourov
// https://www2.cs.arizona.edu/~kobourov/grip_demo.pdf.
template <GraphLike G> class GRIPLayoutAlgorithm {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    uint32_t placementKNearest = 10;
    double simplexExtent = 100000.0;
    typename GRIPPhysicsSystem<G>::Configuration physicsConfig;

    Configuration WithPlacementKNearest(uint32_t v) { placementKNearest = v; return *this; }
    Configuration WithSimplexExtent(double v) { simplexExtent = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Placement K-Nearest", &placementKNearest, ConfigFieldType::UInt32, 1.0, 64.0},
          {"Simplex Extent", &simplexExtent, ConfigFieldType::Double, 1.0, 1000000.0},
      };
    }
  };

  GRIPLayoutAlgorithm(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  void RunFiltration();
  uint32_t TransitionState();
  inline void Tick();

private:
  void placeLayer();

  Configuration *m_config;
  uint32_t m_currLayer;
  DenseNodeSet *m_visible;
  uint8_t m_dimension;
  G *m_graph;
  NestedFiltrationResult *m_filtrationOutput;
  std::unique_ptr<MisFiltrationSystem<G>> m_filtrationSystem = nullptr;
  std::unique_ptr<PositionBarrycentric<G>> m_placementSystem = nullptr;
  std::unique_ptr<GRIPPhysicsSystem<G>> m_physicsSystem = nullptr;
#ifdef GVIZ_DEBUG_CHARTS
  ChartRecorder *m_chart = nullptr;
#endif
};

#include "layout/algorithms/grip.tpp"
