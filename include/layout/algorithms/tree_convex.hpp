#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ds/state_machine.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/animation.hpp"
#include "layout/components/convex_edge_decorator.hpp"
#include "layout/components/convex_node_decorator.hpp"
#include "layout/types.hpp"
#include <cstdint>
#include <vector>

template <GraphLike G> class TreeConvexLayoutAlgorithm {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double edgeLength = 100.0;
    double minSeparation = 40.0;
    double depthDecay = 0.7;
    double subtreeSizeScale = 1.0;
    uint8_t dimension = 2;

    Configuration WithEdgeLength(double v) { edgeLength = v; return *this; }
    Configuration WithMinSeparation(double v) { minSeparation = v; return *this; }
    Configuration WithDepthDecay(double v) { depthDecay = v; return *this; }
    Configuration WithSubtreeSizeScale(double v) { subtreeSizeScale = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Edge Length", &edgeLength, ConfigFieldType::Double, 1.0, 2000.0},
          {"Min Separation", &minSeparation, ConfigFieldType::Double, 0.0, 500.0},
          {"Depth Decay", &depthDecay, ConfigFieldType::Double, 0.0, 1.0},
          {"Subtree Size Scale", &subtreeSizeScale, ConfigFieldType::Double, 0.0, 5.0},
      };
    }
  };

  TreeConvexLayoutAlgorithm(G &graph, bool respectEmbedding, Configuration *config);

  Configuration *Config() { return m_config; }

  void Layout(NodeID root = NodeID{0xffffffff});
  void Tick();

  double AngularResolution() const { return m_resolution; }

  void SetEdgeLengthFunction(EdgeLengthMode mode);
  EdgeLengthMode EdgeLengthFunction() const { return m_edgeLengthMode->Current(); }

  void TransitionTo(EdgeLengthMode mode, double durationSeconds,
                    Easing easing = Easing::EaseInOutCubic);

  PositionAnimationSystem<G> &Animation() { return m_animation; }

  DecoratedGraph<G> &Decorated() { return m_dg; }

private:
  struct ForkSegment {
    uint32_t begin;
    uint32_t end;
  };

  NodeID ChooseRoot() const;
  std::vector<AdjEntry> Children(DenseNodeID v) const;
  void BuildParents(NodeID root);
  void ClassifyShapes();
  void ReorderForMinimumForks();
  void RotateRootChildren();
  std::vector<ForkSegment>
  PartitionForks(const std::vector<AdjEntry> &children) const;
  bool IsTripleRake() const;

  void AssignTrivialPath();
  void AssignTrivialRake();
  void AssignTripleRake();
  void AssignGeneral();

  double AssignSlopesAt(DenseNodeID v, double currentSlope);
  void AssignRakeSlopes(DenseNodeID w, double lo, double hi);
  void AssignChainSlope(DenseNodeID w, double slope, bool allowBend);

  void PlaceVertices();
  void PlacePreorder(DenseNodeID v, double x, double y);

  uint8_t Depth(DenseNodeID v) const;
  double EdgeLengthFor(DenseNodeID topVertex) const;

  Configuration *m_config;
  DecoratedGraph<G> m_dg;
  G *m_graph;
  bool m_respectEmbedding;
  DenseComponentPool<ConvexNodeDecorator> *m_info;
  DenseComponentPool<ConvexEdgeDecorator> *m_edgeInfo;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseNodeID m_root{0};
  uint32_t m_forkCount = 0;
  double m_angleStep = 0.0;
  double m_resolution = 0.0;
  double m_placementLength = 0.0;
  StateMachine<EdgeLengthMode> *m_edgeLengthMode;
  PositionAnimationSystem<G> m_animation;
  uint64_t m_lastConfigHash = 0;
  bool m_hasRun = false;
};

#include "layout/algorithms/tree_convex.tpp"
