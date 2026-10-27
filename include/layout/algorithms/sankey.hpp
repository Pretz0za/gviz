#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "layout/components/cubic_bezier.hpp"
#include "layout/components/half_width.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "layout/components/sankey_node.hpp"
#include "layout/types.hpp"
#include <cstdint>
#include <vector>

template <GraphLike G> class SankeyLayoutAlgorithm {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double columnSpacing = 220.0;
    double nodeSpacing = 24.0;
    double nodeWidth = 16.0;
    double thicknessScale = 1.0;
    double curvature = 0.5;

    Configuration WithColumnSpacing(double v) { columnSpacing = v; return *this; }
    Configuration WithNodeSpacing(double v) { nodeSpacing = v; return *this; }
    Configuration WithNodeWidth(double v) { nodeWidth = v; return *this; }
    Configuration WithThicknessScale(double v) { thicknessScale = v; return *this; }
    Configuration WithCurvature(double v) { curvature = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Column Spacing", &columnSpacing, ConfigFieldType::Double, 20.0, 2000.0},
          {"Node Spacing", &nodeSpacing, ConfigFieldType::Double, 0.0, 400.0},
          {"Node Width", &nodeWidth, ConfigFieldType::Double, 1.0, 200.0},
          {"Thickness Scale", &thicknessScale, ConfigFieldType::Double, 0.01, 50.0},
          {"Curvature", &curvature, ConfigFieldType::Double, 0.0, 1.0},
      };
    }
  };

  SankeyLayoutAlgorithm(G &graph, Configuration *config);

  Configuration *Config() { return m_config; }

  DecoratedGraph<G> &Decorated() { return m_dg; }

  void Layout();
  void Tick();

  uint32_t LayerCount() const { return m_layerCount; }

private:
  void AssignLayers();
  void OrderWithinLayers();
  void ComputeExtents();
  void PlaceNodes();
  void AssignPortsAndBuildCurves();

  double EdgeWeight(EdgeID id) const;
  double EdgeHalfWidth(EdgeID id) const;

  Configuration *m_config;
  DecoratedGraph<G> m_dg;
  G *m_graph;
  DenseComponentPool<SankeyNodeDecorator> *m_info;
  DenseComponentPool<CubicBezierComponent> *m_curves;
  DenseComponentPool<HalfWidthComponent> *m_halfWidths;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<RadiusComponent> *m_radii;
  uint32_t m_layerCount = 0;
  std::vector<std::vector<DenseNodeID>> m_layers;
  std::vector<double> m_columnX;
  uint64_t m_lastConfigHash = 0;
  bool m_hasRun = false;
};

#include "layout/algorithms/sankey.tpp"
