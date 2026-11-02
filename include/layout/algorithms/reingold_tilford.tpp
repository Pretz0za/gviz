#pragma once

#include "concept/graphLike.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/decorations/tree.hpp"
#include "graph/types.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include "layout/types.hpp"
#include "reingold_tilford.hpp"
#include <cstdint>
#include <generator>
#include <stdexcept>

template <GraphLike G>
ReingoldTilfordLayoutAlgorithm<G>::ReingoldTilfordLayoutAlgorithm(
    G &graph, NodeID root, Configuration *config)
    : m_config(config), m_view(&graph), m_stateMachine{States::Initial},
      m_dg(m_view), m_stack{}, m_placedRoots{}, m_tree(nullptr),
      m_rtDecorators(m_dg.nodeSpace.template GetPool<RTNodeDecorator>()),
      m_treeDecorators(m_dg.nodeSpace.template GetPool<TreeNodeDecorator>()),
      m_visible(
          &m_dg.template SetResource<VisibleNodesResource>(graph.size(), 0)) {
  if (!SetRoot(root))
    throw std::runtime_error(
        "ReingoldTilfordLayoutAlgorithm: input graph is not a tree");

  m_stateMachine.AddTransition(States::Initial, States::Terminal); // empty tree
  m_stateMachine.AddTransition(States::Initial, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::Comparison);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::Comparison, States::Comparison);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::Terminal);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::SetRoot(NodeID id) {
  bool out = Decorations::DecorateTree(m_dg, id);
  if (out) {
    m_tree = m_dg.template GetResource<TreeCertificateResource>();
    return true;
  };

  m_tree = nullptr;
  return false;
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::PlaceNext() {
  switch (m_stateMachine.Current()) {
  case States::Initial: {
    m_stack.push_back(m_tree->root);
    DenseNodeID leaf = expand();
    m_placedRoots->push_back(leaf);

    // TODO: animation and aabb stuff
    return;
  }

  case States::SubtreePlaced: {
    if (m_view.MapToSparse(m_lastSubtreePlaced) == m_tree->root) {
      m_stateMachine.Transition(States::Terminal);
      return;
    }

    DenseNodeID found = expand();
    if (m_stateMachine.Current() == States::Comparison) {
      // place vertices on top of each other and make sure comparison state is
      // set up
      return;
    }
    m_placedRoots->push_back(found);

    // TODO: animation and aabb stuff
    return;
  }

  case States::Comparison: {
    if (m_comparisonState.currsep < 1.0)
      comparisonStep();
    else
      iterateContours();
    return;
  }

  case States::Terminal:
    return;
  }
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::iterateContours() {
  ComparisonState &s = m_comparisonState;
  NodeID &lr = s.lrContour;
  NodeID &rl = s.rlContour;

  if (isContourEnd(lr) || isContourEnd(rl))
    return false;

  // iterate left subtree right contour
  DenseNodeID lrDense = m_view.MapToDense(lr);
  NodeID lrNext = firstChild(lr).other;
  if (!lrNext.IsValid()) {
    lrNext = m_rtDecorators->Find(lrDense.Raw())->threadTo;
  }
  double lStep = m_rtDecorators->Find(lrDense.Raw())->offsets.front();
  s.lOffset += lStep;
  s.currsep -= lStep;
  s.lrContour = lrNext;

  // iterate right subtree left contour
  DenseNodeID rlDense = m_view.MapToDense(rl);
  NodeID rlNext = m_rtDecorators->Find(rlDense.Raw())->threadTo;
  if (!rlNext.IsValid()) {
    rlNext = lastChild(rl);
  }
  double rStep = m_rtDecorators->Find(rlDense.Raw())->offsets.back();
  s.rOffset += rStep;
  s.currsep += rStep;
  s.rlContour = rlNext;

  return true;
}

template <GraphLike G>
NodeID ReingoldTilfordLayoutAlgorithm<G>::getAncestor(NodeID node) {
  DenseNodeID dense = m_view.MapToDense(node);
  NodeID ancestor = m_rtDecorators->Find(dense.Raw())->ancestor;
  AdjEntry adj = findChild(m_comparisonState.leastCommonAncestor, ancestor);
  if (adj.edge.IsValid() && adj.other.IsValid()) {
    return ancestor;
  }
  return m_comparisonState.defualtAncestor;
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::comparisonStep() {
  auto &s = m_comparisonState;
  if (s.currsep < 1.0) {
    NodeID ancestor = getAncestor(s.lrContour);
    uint32_t n = siblingAdjacencyDist(s.leastCommonAncestor, ancestor,
                                      s.currRightSubtreeIdx);
    double splitSeparation = (1.0 - s.currsep) / static_cast<double>(n);
    std::vector<double> &offsets =
        m_rtDecorators->Find(m_view.MapToDense(s.leastCommonAncestor).Raw())
            ->offsets;

    for (uint32_t i = 0; i < n; i++) {
      uint32_t idx = s.currRightSubtreeIdx - i;
      uint32_t mult = n - i;
      offsets[idx] += (splitSeparation * static_cast<double>(mult));
    }
  }
}

template <GraphLike G>
std::generator<AdjEntry>
ReingoldTilfordLayoutAlgorithm<G>::children(NodeID node) const {
  for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
    if (adj.other ==
        m_treeDecorators->Find(m_view.MapToDense(node).Raw())->parent) {
      continue;
    }
    co_yield adj;
  }
  co_return;
}

template <GraphLike G>
uint32_t ReingoldTilfordLayoutAlgorithm<G>::childCount(NodeID node) const {
  return m_view.OutDegree(node) - 1;
}

template <GraphLike G>
AdjEntry ReingoldTilfordLayoutAlgorithm<G>::findChild(NodeID parent,
                                                      NodeID child) const {
  for (const AdjEntry &adj : children(parent)) {
    if (adj.other == child)
      return adj;
  }
  return {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
}

template <GraphLike G>
AdjEntry ReingoldTilfordLayoutAlgorithm<G>::lastChild(NodeID node) const {
  AdjEntry out = {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
  for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
    if (adj.other ==
        m_treeDecorators->Find(m_view.MapToDense(node).Raw())->parent) {
      continue;
    }
    out = adj;
  }
  return out;
}

template <GraphLike G>
AdjEntry ReingoldTilfordLayoutAlgorithm<G>::firstChild(NodeID node) const {
  for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
    if (adj.other ==
        m_treeDecorators->Find(m_view.MapToDense(node).Raw())->parent) {
      continue;
    }
    return adj;
  }
  return {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
}

template <GraphLike G>
AdjEntry ReingoldTilfordLayoutAlgorithm<G>::isLeaf(NodeID node) const {
  return childCount(node) == 0;
}

template <GraphLike G>
uint32_t ReingoldTilfordLayoutAlgorithm<G>::siblingAdjacencyDist(
    NodeID parent, NodeID child1, NodeID child2) const {
  uint32_t l, r, curr = 0;
  for (const AdjEntry &adj : children(parent)) {
    if (adj.other == child1)
      l = curr;
    else if (adj.other == child2)
      r = curr;
    curr++;
  }
  return r - l;
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::isContourEnd(NodeID node) const {
  if (!isLeaf(node))
    return false;

  DenseNodeID dense = m_view.MapToDense(node);
  return !m_rtDecorators->Find(dense.Raw())->threadTo.IsValid();
}

template <GraphLike G> DenseNodeID ReingoldTilfordLayoutAlgorithm<G>::expand() {
  NodeID target = m_stack.back();
  AdjEntry probe = firstChild(target);

  // Base Case 1: leaf node
  if (!probe.edge.IsValid() || !probe.other.IsValid()) {
    DenseNodeID dense = m_view.MapToDense(target);
    m_visible->Set(dense);
    m_stack.pop_back();

    solveLeafDecorators(dense);

    m_stateMachine.Transition(States::SubtreePlaced);
    m_lastSubtreePlaced = dense;
    return dense;
  }

  DenseNodeID dProbe = m_view.MapToDense(probe.other);
  if (m_visible->Test(dProbe)) {
    // Base Case 2: children placed, merge and mark parent placed
    if (dProbe == m_lastSubtreePlaced) {
      DenseNodeID dense = m_view.MapToDense(target);
      solveSingleChildDecorators(dense);
      m_visible->Set(dense);
      m_stack.pop_back();
      m_stateMachine.Transition(States::SubtreePlaced);
      m_lastSubtreePlaced = dense;
      return dense;
    } else {
      m_stateMachine.Transition(States::Comparison);
      m_stack.pop_back();
      m_comparisonState.leastCommonAncestor = target;
      m_comparisonState.generator = children(target);
      m_comparisonState.it = m_comparisonState.generator.begin();
      m_comparisonState.currRightSubtreeIdx = 0;
      m_comparisonState.iterate();
      return m_view.MapToDense(target);
    }
  } else {
    size_t l = m_stack.size();
    for (const AdjEntry &adj : children(target)) {
      m_stack.push_back(adj.other);
    }

    // reverse m_stack between [l, r) where r = size()
    std::reverse(m_stack.begin() + l, m_stack.end());
    expand();
  }
}
