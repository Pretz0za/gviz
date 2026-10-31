#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "graph/undirected_view.hpp"
template <GraphLike G> class ReingoldTilfordLayoutAlgorithm {
public:
  using View = UndirectedView<G>;

  class Configuration : IConfiguration {
  public:
    Configuration() = default;

    double min_separation = 1000.0;
    double layer_separation = 500.0;
    uint8_t dimension = 2;

    Configuration WithMinSeparation(double newSep) {
      min_separation = newSep;
      return *this;
    }
    Configuration WtihLayerSeparation(double newSep) {
      layer_separation = newSep;
      return *this;
    }

    std::vector<ConfigField> Fields() override {
      return {
          {"Minimum Separation", &min_separation, ConfigFieldType::Double,
           100.0, 10000.0},
          {"Layer Separation", &layer_separation, ConfigFieldType::Double,
           100.0, 10000.0},
      };
    }
  };

  ReingoldTilfordLayoutAlgorithm<G>(G &graph, NodeID root, Configuration *config);

  // detect a root?
  // ReingoldTilfordLayoutAlgorithm<G>(G &graph, Configuration *config);

  void SetRoot(NodeID newRoot);
  void PlaceAll();
  void PlaceNext();

  void Tick();

private:
  DecoratedGraph<View> m_dg;
  Configuration *m_config;


};
