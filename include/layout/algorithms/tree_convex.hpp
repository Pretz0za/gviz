#pragma once

#include "concept/graphLike.hpp"
#include "ds/state_machine.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
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
  TreeConvexLayoutAlgorithm(G &graph, bool respectEmbedding);

  void Layout(NodeID root = NodeID{0xffffffff});

  double AngularResolution() const { return m_resolution; }
  void SetEdgeLength(double length) { m_edgeLength = length; }
  void SetMinimumLeafSeparation(double separation) {
    m_minSeparation = separation;
  }

  void SetEdgeLengthFunction(EdgeLengthMode mode);
  EdgeLengthMode EdgeLengthFunction() const { return m_edgeLengthMode->Current(); }

  void TransitionTo(EdgeLengthMode mode, double durationSeconds,
                    Easing easing = Easing::EaseInOutCubic);

  PositionAnimationSystem<G> &Animation() { return m_animation; }

  void SetDepthDecay(double decay) { m_depthDecay = decay; }
  void SetSubtreeSizeScale(double scale) { m_subtreeSizeScale = scale; }

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

  G *m_graph;
  bool m_respectEmbedding;
  DenseComponentPool<ConvexNodeDecorator> *m_info;
  DenseComponentPool<ConvexEdgeDecorator> *m_edgeInfo;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseNodeID m_root{0};
  uint32_t m_forkCount = 0;
  double m_angleStep = 0.0;
  double m_resolution = 0.0;
  double m_edgeLength = 100.0;
  double m_minSeparation = 40.0;
  double m_placementLength = 0.0;
  StateMachine<EdgeLengthMode> *m_edgeLengthMode;
  double m_depthDecay = 0.7;
  double m_subtreeSizeScale = 1.0;
  PositionAnimationSystem<G> m_animation;
};

#include "layout/algorithms/tree_convex.tpp"
