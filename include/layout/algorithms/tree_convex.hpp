#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radial_edge_decorator.hpp"
#include "layout/components/radial_node_decorator.hpp"
#include <cstdint>
#include <vector>

template <GraphLike G> class TreeConvexLayoutAlgorithm {
public:
  TreeConvexLayoutAlgorithm(G &graph, bool respectEmbedding);

  void Layout();

  double AngularResolution() const { return m_resolution; }
  void SetEdgeLength(double length) { m_edgeLength = length; }
  void SetMinimumLeafSeparation(double separation) {
    m_minSeparation = separation;
  }

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

  G *m_graph;
  bool m_respectEmbedding;
  DenseComponentPool<RadialNodeDecorator> *m_info;
  DenseComponentPool<RadialEdgeDecorator> *m_edgeInfo;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseNodeID m_root{0};
  uint32_t m_forkCount = 0;
  double m_angleStep = 0.0;
  double m_resolution = 0.0;
  double m_edgeLength = 100.0;
  double m_minSeparation = 40.0;
  double m_placementLength = 0.0;
};

#include "layout/algorithms/tree_convex.tpp"
