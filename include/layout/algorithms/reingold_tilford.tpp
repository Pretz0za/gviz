#pragma once

#include "concept/graphLike.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/decorations/tree.hpp"
#include "graph/types.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include "layout/types.hpp"
#include "reingold_tilford.hpp"
#include <generator>
#include <stdexcept>

template <GraphLike G>
ReingoldTilfordLayoutAlgorithm<G>::ReingoldTilfordLayoutAlgorithm(
    G &graph, NodeID root, Configuration *config)
    : m_config(config), m_view(&graph), m_stateMachine{States::Initial},
      m_dg(m_view), m_stack{}, m_placedRoots{graph.size(), 0}, m_tree(nullptr),
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
    m_placedRoots->Set(leaf);

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
      comparisonStep();
      return;
    }
    m_placedRoots->Set(found);

    // TODO: animation and aabb stuff
    return;
  }

  case States::Comparison: {
    comparisonStep();
    return;
  }

  case States::Terminal:
    return;
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::comparisonStep() {}

template <GraphLike G>
std::generator<AdjEntry>
ReingoldTilfordLayoutAlgorithm<G>::children(NodeID node) const {
  for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
    if (adj.other == m_treeDecorators->Find(m_view.MapToSparse(node))->parent) {
      continue;
    }
    co_yield adj;
  }
  co_return;
}

template <GraphLike G>
AdjEntry ReingoldTilfordLayoutAlgorithm<G>::firstChild(NodeID node) const {
  for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
    if (adj.other == m_treeDecorators->Find(m_view.MapToSparse(node))->parent) {
      continue;
    }
    return adj;
  }
  return {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
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
      m_comparisonState.generator = children(target);
      m_comparisonState.it = m_comparisonState.generator.begin();
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
