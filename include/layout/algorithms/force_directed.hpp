#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "debug/chart_recorder.hpp"
#include "ds/quadtree.hpp"
#include "layout/algorithms/types.hpp"
#include "layout/components/mass.hpp"
#include "layout/components/position.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/heat/force_atlas.hpp"
#include "layout/physics/gravity.hpp"
#include "layout/placement/randomized.hpp"
#include <cstdint>
#include <memory>

template <GraphLike G, ForceModel F> class ForceDirectedLayoutAlgorithm {

public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    // Barnes-Hut opening-angle threshold: a quadtree node is treated as a
    // single mass once (2 * halfLength / distance) falls below this.
    double theta = 1.0;
    typename F::Configuration forceModelConfig;
    typename GravityForceSystem<G>::Configuration gravityConfig;
    typename PositionRandomized<G>::Configuration randomizedConfig;
    typename ForceAtlasHeat<G>::Configuration heatConfig;

    Configuration WithTheta(double v) { theta = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Theta", &theta, ConfigFieldType::Double, 0.0, 2.0},
      };
    }
  };

  ForceDirectedLayoutAlgorithm(G &graph, Configuration *config);
  void Tick();

  Configuration *Config() { return m_config; }

private:
  void RepulsiveTick(SpacialIndex::QuadTree<DenseNodeID, 1> *node, DenseNodeID v);
  void RebuildQuadTree();
  Configuration *m_config;
  G *m_graph;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<MassComponent> *m_mass;
  std::unique_ptr<F> m_forceModel;
  std::unique_ptr<GravityForceSystem<G>> m_gravity;
  std::unique_ptr<PositionRandomized<G>> m_randomizer;
  std::unique_ptr<ForceAtlasHeat<G>> m_heat;
  QuadTreeResource *m_quadtree;
  uint8_t m_dimension;
#ifdef GVIZ_DEBUG_CHARTS
  ChartRecorder *m_chart = nullptr;
#endif
};

#include "layout/algorithms/force_directed.tpp"
