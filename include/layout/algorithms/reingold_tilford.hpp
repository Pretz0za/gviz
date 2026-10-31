#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/decorations/components/tree_node_decorator.hpp"
#include "graph/types.hpp"
#include "graph/undirected_view.hpp"
#include "layout/components/rt_node_decorator.hpp"
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

  ReingoldTilfordLayoutAlgorithm<G>(G &graph, NodeID root,
                                    Configuration *config);

  // detect a root?
  // ReingoldTilfordLayoutAlgorithm<G>(G &graph, Configuration *config);

  bool SetRoot(NodeID newRoot);
  void PlaceAll();
  void PlaceNext();

  void Tick();

private:
  DenseNodeID expand();

  std::generator<AdjEntry> children(NodeID node);
  AdjEntry firstChild(NodeID node);
  void initializeLeafDecorators(DenseNodeID node);

  View m_view;
  DecoratedGraph<View> m_dg;
  Configuration *m_config;

  DenseNodeSet *m_visible;
  std::vector<DenseNodeID> m_placedRoots;
  std::vector<NodeID> m_stack;

  DenseComponentPool<RTNodeDecorator> *m_rtDecorators;
  DenseComponentPool<TreeNodeDecorator> *m_treeDecorators;
};

#include "reingold_tilford.tpp"
