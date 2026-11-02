#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ds/state_machine.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "graph/undirected_tree_view.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include <utility>
template <GraphLike G> class ReingoldTilfordLayoutAlgorithm {
public:
  using View = UndirectedTreeView<G>;

  enum class States : uint8_t { Initial, SubtreePlaced, Comparison, Terminal };

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
  struct ComparisonState {
    std::generator<AdjEntry> generator;
    std::ranges::iterator_t<std::generator<AdjEntry>> it;
    AdjEntry prevSibling;
    NodeID leastCommonAncestor;
    NodeID defualtAncestor;
    NodeID lrContour;
    NodeID rlContour;
    uint32_t currRightSubtreeIdx;
    double lOffset = 0.0;
    double rOffset = 0.0;
    double currsep = 0.0;

    bool iterate() {
      prevSibling = *it;
      it++;
      if (it == generator.end())
        return false;
      lrContour = prevSibling.other;
      rlContour = (*it).other;
      lOffset = 0.0;
      rOffset = 0.0;
      currRightSubtreeIdx++;
      return true;
    }
  };

  DenseNodeID expand();

  bool isContourEnd(NodeID node) const;

  NodeID getAncestor(NodeID node);
  void comparisonStep();
  bool iterateContours();

  void solveLeafDecorators(DenseNodeID node);
  void solveSingleChildDecorators(DenseNodeID node);

  DecoratedGraph<G> m_dg;
  View m_view;
  Configuration *m_config;

  DenseNodeSet *m_visible;
  std::vector<DenseNodeID> *m_placedRoots;
  std::vector<NodeID> m_stack;

  DenseComponentPool<RTNodeDecorator> *m_rtDecorators;

  StateMachine<States> m_stateMachine;
  DenseNodeID m_lastSubtreePlaced = INVALID_DENSE_NODE_ID;
  ComparisonState m_comparisonState;
};

#include "reingold_tilford.tpp"
