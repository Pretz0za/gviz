#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ds/state_machine.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "graph/undirected_tree_view.hpp"
#include "layout/animation.hpp"
#include "layout/components/animation_position.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include "render/components/color.hpp"
#include <optional>
#include <utility>

template <GraphLike G> class ReingoldTilfordLayoutAlgorithm {
public:
  using View = UndirectedTreeView<G>;

  enum class States : uint8_t { Initial, SubtreePlaced, Comparison, Terminal };

  class Configuration : public IConfiguration {
  public:
    enum class Direction : uint8_t { Down, Up, Left, Right };

    Configuration() = default;

    double min_separation = 1000.0;
    double layer_separation = 500.0;
    double step_animation_seconds = 0.3;
    double radius = 20.0;
    Direction direction = Direction::Down;
    uint8_t dimension = 2;

    Configuration WithMinSeparation(double newSep) {
      min_separation = newSep;
      return *this;
    }
    Configuration WithLayerSeparation(double newSep) {
      layer_separation = newSep;
      return *this;
    }
    Configuration WithStepAnimationSeconds(double seconds) {
      step_animation_seconds = seconds;
      return *this;
    }
    Configuration WithRadius(double newRadius) {
      radius = newRadius;
      return *this;
    }
    Configuration WithDirection(Direction newDirection) {
      direction = newDirection;
      return *this;
    }

    std::vector<ConfigField> Fields() override {
      return {
          {"Minimum Separation", &min_separation, ConfigFieldType::Double,
           100.0, 10000.0},
          {"Layer Separation", &layer_separation, ConfigFieldType::Double,
           100.0, 10000.0},
          {"Step Animation Seconds", &step_animation_seconds,
           ConfigFieldType::Double, 0.0, 5.0},
          {"Vertex Radius", &radius, ConfigFieldType::Double, 1.0, 200.0},
          {"Direction", &direction, ConfigFieldType::UInt8, 0.0, 3.0},
      };
    }
  };

  ReingoldTilfordLayoutAlgorithm(G &graph, NodeID root, Configuration *config);

  bool SetRoot(NodeID newRoot);

  void PlaceAll();
  void PlaceNext();
  void Tick();

  bool Done() const { return m_stateMachine.Current() == States::Terminal; }

  Configuration *Config() { return m_config; }
  DecoratedGraph<G> &Decorated() { return m_dg; }

private:
  static constexpr uint32_t kPlacedColor = 0x4D8CD9FFu;
  static constexpr uint32_t kJustPlacedColor = 0x34C759FFu;
  static constexpr uint32_t kComparingColor = 0xFF9F2EFFu;

  struct ComparisonState {
    std::generator<AdjEntry> generator;
    std::ranges::iterator_t<std::generator<AdjEntry>> it;
    AdjEntry prevSibling;
    NodeID leastCommonAncestor;
    NodeID defaultAncestor;
    NodeID lrContour;
    NodeID rlContour;
    uint32_t currRightSubtreeIdx = 0;
    double lOffset = 0.0;
    double rOffset = 0.0;
    double currsep = 0.0;
    double totalNewSeparation = 0.0;

    std::vector<NodeID> abandonedLMosts;

    explicit ComparisonState(std::generator<AdjEntry> gen)
        : generator(std::move(gen)), it(generator.begin()) {}

    bool iterate() {
      prevSibling = *it;
      it++;
      lrContour = prevSibling.other;
      rlContour = (*it).other;
      lOffset = 0.0;
      rOffset = 0.0;
      currsep = 0.0;
      currRightSubtreeIdx++;
      totalNewSeparation = 0.0;

      return it != generator.end();
    }
  };

  void advanceOneStep();
  void handleExpand(DenseNodeID found);
  void removePlacedChildren(DenseNodeID found);
  void beginComparison();
  void advanceComparison();
  void finishPlacement(DenseNodeID node);
  bool isRoot(DenseNodeID node) const;

  void highlightComparison(NodeID a, NodeID b);
  void clearComparisonHighlight();

  DenseNodeID expand();

  bool isContourEnd(NodeID node) const;

  NodeID getAncestor(NodeID node);
  void comparisonStep();
  void updateExtent();
  void markAncestorAlongRightContour(NodeID start);
  bool comparisonConclude();
  bool iterateContours();

  void solveLeafDecorators(DenseNodeID node, uint32_t depth);
  void solveSingleChildDecorators(DenseNodeID node, uint32_t depth);

  void writePositions();
  void placePreorder(NodeID node, double x, NodeID skipChild);
  void writeAxes(PositionComponent *pos, double spread, double depthValue) const;
  void snapToFinalPosition(DenseNodeID node);

  DecoratedGraph<G> m_dg;
  View m_view;
  Configuration *m_config;

  DenseNodeSet *m_visible;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<AnimationFromPositionComponent> *m_animFrom;
  DenseComponentPool<AnimationToPositionComponent> *m_animTo;
  DenseComponentPool<RadiusComponent> *m_radii;
  DenseComponentPool<ColorComponent> *m_colors;
  PositionAnimationSystem<G> m_animation;

  std::vector<DenseNodeID> m_placedRoots;
  std::vector<NodeID> m_stack;
  std::vector<uint32_t> m_depthStack;

  DenseComponentPool<RTNodeDecorator> *m_rtDecorators;

  StateMachine<States> m_stateMachine;
  DenseNodeID m_lastSubtreePlaced = INVALID_DENSE_NODE_ID;
  DenseNodeID m_justPlacedHighlight = INVALID_DENSE_NODE_ID;
  DenseNodeID m_newlyRevealed = INVALID_DENSE_NODE_ID;
  NodeID m_highlightA = INVALID_NODE_ID;
  NodeID m_highlightB = INVALID_NODE_ID;
  std::optional<ComparisonState> m_comparisonState;

  uint64_t m_lastConfigHash = 0;
  bool m_configInitialized = false;
};

#include "reingold_tilford.tpp"
