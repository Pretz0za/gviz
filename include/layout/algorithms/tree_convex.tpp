#pragma once

#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/tree_convex.hpp"
#include "layout/types.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

template <GraphLike G>
TreeConvexLayoutAlgorithm<G>::TreeConvexLayoutAlgorithm(G &graph,
                                                        bool respectEmbedding)
    : m_graph(&graph), m_respectEmbedding(respectEmbedding) {
  m_info = graph.NodeSpace().template SetPool<RadialNodeDecorator>();
  m_edgeInfo = graph.EdgeSpace().template SetPool<RadialEdgeDecorator>();

  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  uint8_t dimension = static_cast<uint8_t>(*dim);
  m_positions =
      graph.NodeSpace().template SetPool<PositionComponent>(dimension);
}

template <GraphLike G> NodeID TreeConvexLayoutAlgorithm<G>::ChooseRoot() const {
  NodeID best = INVALID_NODE_ID;
  uint32_t bestDegree = 0;
  for (NodeID id : m_graph->Nodes()) {
    uint32_t degree = m_graph->OutDegree(id);
    if (degree > bestDegree) {
      bestDegree = degree;
      best = id;
    }
  }
  return best;
}

template <GraphLike G>
std::vector<AdjEntry> TreeConvexLayoutAlgorithm<G>::Children(DenseNodeID v) const {
  NodeID parent = m_info->Find(v.Raw())->parent;
  std::vector<AdjEntry> result;
  for (const AdjEntry &adj : m_graph->OutNeighbors(m_graph->MapToSparse(v)))
    if (adj.other != parent)
      result.push_back(adj);

  std::ranges::sort(result, {}, [this](const AdjEntry &adj) {
    return m_edgeInfo->Find(adj.edge.Raw())->order;
  });
  return result;
}

template <GraphLike G>
void TreeConvexLayoutAlgorithm<G>::BuildParents(NodeID root) {
  uint32_t n = m_graph->Size();
  DenseNodeSet visited(n, false);
  std::vector<NodeID> stack;
  stack.push_back(root);
  visited.Set(m_graph->MapToDense(root));

  while (!stack.empty()) {
    NodeID current = stack.back();
    stack.pop_back();
    uint32_t order = 0;
    for (const AdjEntry &adj : m_graph->OutNeighbors(current)) {
      DenseNodeID denseOther = m_graph->MapToDense(adj.other);
      if (visited.Test(denseOther))
        continue;
      visited.Set(denseOther);
      m_info->Find(denseOther.Raw())->parent = current;
      m_edgeInfo->Find(adj.edge.Raw())->order = order++;
      stack.push_back(adj.other);
    }
  }
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::ClassifyShapes() {
  std::vector<DenseNodeID> order;
  std::vector<DenseNodeID> stack;
  stack.push_back(m_root);

  while (!stack.empty()) {
    DenseNodeID id = stack.back();
    stack.pop_back();
    order.push_back(id);
    for (const AdjEntry &adj : Children(id))
      stack.push_back(m_graph->MapToDense(adj.other));
  }

  for (auto it = order.rbegin(); it != order.rend(); ++it) {
    DenseNodeID id = *it;
    RadialNodeDecorator *info = m_info->Find(id.Raw());
    std::vector<AdjEntry> children = Children(id);
    uint32_t k = static_cast<uint32_t>(children.size());

    uint32_t branchy = 0;
    bool anyOther = false;
    for (const AdjEntry &adj : children) {
      RadialNodeDecorator *child =
          m_info->Find(m_graph->MapToDense(adj.other).Raw());
      if (child->shape == TreeShape::Other)
        anyOther = true;
      else if (child->shape == TreeShape::Rake || !child->chainable)
        branchy++;
    }

    if (anyOther || k > 3 || branchy > 2) {
      info->shape = TreeShape::Other;
      info->chainable = false;
    } else if (branchy == 0) {
      if (k <= 2) {
        info->shape = TreeShape::Path;
        info->chainable = (k <= 1);
      } else {
        info->shape = TreeShape::Rake;
        info->chainable = false;
      }
    } else {
      info->shape = TreeShape::Rake;
      info->chainable = (k <= 1);
    }
  }
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::ReorderForMinimumForks() {
  uint32_t n = m_graph->Size();
  for (uint32_t i = 0; i < n; i++) {
    std::vector<AdjEntry> children = Children(DenseNodeID{i});
    if (children.empty())
      continue;

    std::vector<AdjEntry> paths, rakes, others;
    for (const AdjEntry &adj : children) {
      TreeShape shape = m_info->Find(m_graph->MapToDense(adj.other).Raw())->shape;
      if (shape == TreeShape::Path)
        paths.push_back(adj);
      else if (shape == TreeShape::Rake)
        rakes.push_back(adj);
      else
        others.push_back(adj);
    }

    bool pathsAreMajority = paths.size() >= others.size();
    std::vector<AdjEntry> &majority = pathsAreMajority ? paths : others;
    std::vector<AdjEntry> &minority = pathsAreMajority ? others : paths;

    std::vector<AdjEntry> reordered;
    reordered.reserve(children.size());
    size_t mi = 0, ni = 0;
    while (mi < majority.size() || ni < minority.size()) {
      if (mi < majority.size())
        reordered.push_back(majority[mi++]);
      if (ni < minority.size())
        reordered.push_back(minority[ni++]);
    }
    for (const AdjEntry &adj : rakes)
      reordered.push_back(adj);

    for (uint32_t pos = 0; pos < reordered.size(); pos++)
      m_edgeInfo->Find(reordered[pos].edge.Raw())->order = pos;
  }
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::RotateRootChildren() {
  std::vector<AdjEntry> children = Children(m_root);
  size_t k = children.size();
  if (k < 2)
    return;

  auto shapeOf = [&](size_t idx) {
    return m_info->Find(m_graph->MapToDense(children[idx].other).Raw())->shape;
  };

  size_t start = 0;
  for (size_t i = 0; i < k; i++) {
    if (shapeOf(i) != TreeShape::Path)
      continue;
    size_t prev = (i + k - 1) % k;
    if (shapeOf(prev) != TreeShape::Rake) {
      start = i;
      break;
    }
  }

  if (start == 0)
    return;

  for (uint32_t pos = 0; pos < k; pos++) {
    const AdjEntry &adj = children[(start + pos) % k];
    m_edgeInfo->Find(adj.edge.Raw())->order = pos;
  }
}

template <GraphLike G>
std::vector<typename TreeConvexLayoutAlgorithm<G>::ForkSegment>
TreeConvexLayoutAlgorithm<G>::PartitionForks(
    const std::vector<AdjEntry> &children) const {
  std::vector<ForkSegment> segments;
  size_t k = children.size();
  size_t i = 0;

  auto shapeOf = [&](size_t idx) {
    return m_info->Find(m_graph->MapToDense(children[idx].other).Raw())->shape;
  };

  while (i < k) {
    if (shapeOf(i) != TreeShape::Path) {
      i++;
      continue;
    }
    size_t j = i + 1;
    while (j < k && shapeOf(j) == TreeShape::Rake)
      j++;
    if (j < k && shapeOf(j) == TreeShape::Path) {
      segments.push_back({static_cast<uint32_t>(i), static_cast<uint32_t>(j)});
      i = j;
    } else {
      i++;
    }
  }
  return segments;
}

template <GraphLike G> bool TreeConvexLayoutAlgorithm<G>::IsTripleRake() const {
  std::vector<AdjEntry> children = Children(m_root);
  if (children.size() != 3)
    return false;
  for (const AdjEntry &adj : children) {
    TreeShape shape = m_info->Find(m_graph->MapToDense(adj.other).Raw())->shape;
    if (shape != TreeShape::Path && shape != TreeShape::Rake)
      return false;
  }
  return true;
}

template <GraphLike G>
void TreeConvexLayoutAlgorithm<G>::AssignChainSlope(DenseNodeID w, double slope,
                                                    bool allowBend) {
  constexpr double pi = std::numbers::pi;
  bool first = true;
  for (const AdjEntry &adj : Children(w)) {
    double edgeSlope = (first || !allowBend) ? slope : slope + pi;
    m_edgeInfo->Find(adj.edge.Raw())->slope = edgeSlope;
    AssignChainSlope(m_graph->MapToDense(adj.other), edgeSlope, allowBend);
    first = false;
  }
}

template <GraphLike G>
void TreeConvexLayoutAlgorithm<G>::AssignRakeSlopes(DenseNodeID w, double lo,
                                                    double hi) {
  std::vector<AdjEntry> children = Children(w);
  size_t k = children.size();
  if (k == 0)
    return;

  double width = (hi - lo) / static_cast<double>(k);
  for (size_t i = 0; i < k; i++) {
    double subLo = lo + static_cast<double>(i) * width;
    double subHi = subLo + width;
    double dir = (subLo + subHi) / 2.0;
    m_edgeInfo->Find(children[i].edge.Raw())->slope = dir;
    AssignRakeSlopes(m_graph->MapToDense(children[i].other), subLo, subHi);
  }
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::AssignTrivialPath() {
  AssignChainSlope(m_root, 0.0, true);
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::AssignTrivialRake() {
  AssignRakeSlopes(m_root, 0.0, 2.0 * std::numbers::pi);
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::AssignTripleRake() {
  constexpr double pi = std::numbers::pi;
  std::vector<AdjEntry> children = Children(m_root);
  constexpr double armStep = 2.0 * pi / 3.0;
  constexpr double halfWedge = 0.45 * armStep;

  for (size_t i = 0; i < children.size(); i++) {
    double armSlope = static_cast<double>(i) * armStep;
    const AdjEntry &adj = children[i];
    m_edgeInfo->Find(adj.edge.Raw())->slope = armSlope;
    DenseNodeID child = m_graph->MapToDense(adj.other);
    AssignRakeSlopes(child, armSlope - halfWedge, armSlope + halfWedge);
  }
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::AssignGeneral() {
  AssignSlopesAt(m_root, 0.0);
}

template <GraphLike G>
double TreeConvexLayoutAlgorithm<G>::AssignSlopesAt(DenseNodeID v,
                                                    double currentSlope) {
  RadialNodeDecorator *info = m_info->Find(v.Raw());
  std::vector<AdjEntry> children = Children(v);
  std::vector<ForkSegment> segments = PartitionForks(children);

  double firstSlope = currentSlope;
  double lastSlope = currentSlope;
  bool haveFirst = false;

  size_t k = children.size();
  size_t segIdx = 0;
  size_t i = 0;
  size_t lastForkEnd = k + 1;

  while (i < k) {
    bool startsSegment =
        segIdx < segments.size() && segments[segIdx].begin == i;

    if (!startsSegment && lastForkEnd == i) {
      i++;
      continue;
    }

    if (startsSegment) {
      bool sharedOpening = (lastForkEnd == i);
      ForkSegment seg = segments[segIdx++];

      if (!sharedOpening) {
        const AdjEntry &firstPath = children[seg.begin];
        m_edgeInfo->Find(firstPath.edge.Raw())->slope = currentSlope;
        AssignRakeSlopes(m_graph->MapToDense(firstPath.other), currentSlope,
                         currentSlope + m_angleStep / 2.0);
        if (!haveFirst) {
          firstSlope = currentSlope;
          haveFirst = true;
        }
      }

      for (uint32_t m = seg.begin + 1; m < seg.end; m++) {
        const AdjEntry &rake = children[m];
        double theta1 = currentSlope;
        double theta2 = currentSlope + m_angleStep;
        m_edgeInfo->Find(rake.edge.Raw())->slope = theta2;
        AssignRakeSlopes(m_graph->MapToDense(rake.other), theta1, theta2);
        currentSlope += m_angleStep;
      }

      currentSlope += m_angleStep;
      const AdjEntry &lastPath = children[seg.end];
      m_edgeInfo->Find(lastPath.edge.Raw())->slope = currentSlope;
      AssignRakeSlopes(m_graph->MapToDense(lastPath.other),
                       currentSlope - m_angleStep / 2.0, currentSlope);
      lastSlope = currentSlope;

      lastForkEnd = seg.end;
      i = seg.end;
      continue;
    }

    const AdjEntry &adj = children[i];
    DenseNodeID child = m_graph->MapToDense(adj.other);
    TreeShape shape = m_info->Find(child.Raw())->shape;

    if (shape == TreeShape::Path) {
      m_edgeInfo->Find(adj.edge.Raw())->slope = currentSlope;
      AssignRakeSlopes(child, currentSlope, currentSlope + m_angleStep / 2.0);
      if (!haveFirst) {
        firstSlope = currentSlope;
        haveFirst = true;
      }
      lastSlope = currentSlope;
    } else if (shape == TreeShape::Rake) {
      double theta1 = currentSlope;
      double theta2 = currentSlope + m_angleStep;
      m_edgeInfo->Find(adj.edge.Raw())->slope = theta2;
      AssignRakeSlopes(child, theta1, theta2);
      if (!haveFirst) {
        firstSlope = theta1;
        haveFirst = true;
      }
      currentSlope += m_angleStep;
      lastSlope = currentSlope;
    } else {
      double childEnd = AssignSlopesAt(child, currentSlope);
      RadialNodeDecorator *childInfo = m_info->Find(child.Raw());
      double bisect =
          (childInfo->firstLeafSlope + childInfo->lastLeafSlope) / 2.0;
      m_edgeInfo->Find(adj.edge.Raw())->slope = bisect;
      if (!haveFirst) {
        firstSlope = childInfo->firstLeafSlope;
        haveFirst = true;
      }
      lastSlope = childInfo->lastLeafSlope;
      currentSlope = childEnd;
    }
    i++;
  }

  info->firstLeafSlope = firstSlope;
  info->lastLeafSlope = lastSlope;
  return currentSlope;
}

template <GraphLike G>
void TreeConvexLayoutAlgorithm<G>::PlacePreorder(DenseNodeID v, double x,
                                                 double y) {
  auto &pos = m_positions->Data();
  for (const AdjEntry &adj : Children(v)) {
    RadialEdgeDecorator *edge = m_edgeInfo->Find(adj.edge.Raw());
    edge->length = m_placementLength;
    DenseNodeID child = m_graph->MapToDense(adj.other);
    double cx = x + edge->length * std::cos(edge->slope);
    double cy = y + edge->length * std::sin(edge->slope);
    pos[child.Raw()].pos[0] = cx;
    pos[child.Raw()].pos[1] = cy;
    PlacePreorder(child, cx, cy);
  }
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::PlaceVertices() {
  m_placementLength = m_resolution > 0.0
                          ? std::max(m_edgeLength, m_minSeparation / m_resolution)
                          : m_edgeLength;

  auto &pos = m_positions->Data();
  pos[m_root.Raw()].pos[0] = 0.0;
  pos[m_root.Raw()].pos[1] = 0.0;
  PlacePreorder(m_root, 0.0, 0.0);
}

template <GraphLike G> void TreeConvexLayoutAlgorithm<G>::Layout() {
  constexpr double pi = std::numbers::pi;
  uint32_t n = m_graph->Size();
  if (n == 0)
    return;

  if (n == 1) {
    auto &pos = m_positions->Data();
    pos[0].pos[0] = 0.0;
    pos[0].pos[1] = 0.0;
    m_resolution = pi;
    return;
  }

  NodeID root = ChooseRoot();
  BuildParents(root);
  m_root = m_graph->MapToDense(root);
  ClassifyShapes();

  TreeShape rootShape = m_info->Find(m_root.Raw())->shape;

  if (rootShape == TreeShape::Path) {
    AssignTrivialPath();
    m_resolution = pi;
  } else if (rootShape == TreeShape::Rake) {
    AssignTrivialRake();
    m_resolution = pi / 2.0;
  } else if (IsTripleRake()) {
    AssignTripleRake();
    m_resolution = pi / 3.0;
  } else {
    if (!m_respectEmbedding)
      ReorderForMinimumForks();
    RotateRootChildren();

    m_angleStep = 1.0;
    double measuredSweep = AssignSlopesAt(m_root, 0.0);
    m_forkCount = static_cast<uint32_t>(std::llround(measuredSweep));

    m_angleStep = m_forkCount > 0 ? (2.0 * pi / m_forkCount) : pi / 2.0;
    AssignGeneral();
    m_resolution = m_angleStep;
  }

  PlaceVertices();
}
