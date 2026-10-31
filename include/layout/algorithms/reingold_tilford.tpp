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
    : m_config(config), m_view(&graph), m_dg(m_view),
      m_rtDecorators(m_dg.nodeSpace.template GetPool<RTNodeDecorator>()),
      m_treeDecorators(m_dg.nodeSpace.template GetPool<TreeNodeDecorator>()),
      m_visible(
          &m_dg.template SetResource<VisibleNodesResource>(graph.size(), 0)) {
  if (!SetRoot(root))
    throw std::runtime_error(
        "ReingoldTilfordLayoutAlgorithm: input graph is not a tree");

  m_stack.push_back(root);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::SetRoot(NodeID id) {
  Decorations::DecorateTree(m_dg, id);
}

template <GraphLike G>
std::generator<AdjEntry>
ReingoldTilfordLayoutAlgorithm<G>::children(NodeID node) {
  for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
    if (adj.other == m_treeDecorators->Find(m_view.MapToSparse(node))->parent) {
      continue;
    }
    co_yield adj;
  }
  co_return;
}

template <GraphLike G>
AdjEntry ReingoldTilfordLayoutAlgorithm<G>::firstChild(NodeID node) {
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

    initializeLeafDecorators(dense);
    return dense;
  }

  DenseNodeID dProbe = m_view.MapToDense(probe.other);
  if (m_visible->Test(dProbe)) {
    // Base Case 2: children placed, merge and mark parent placed
  } else {

    // Recursive case: children not placed, push to stack backwards and recurse
    size_t l = m_stack.size();
    for (const AdjEntry &adj : children(target)) {
      m_stack.push_back(adj.other);
    }

    // reverse m_stack between [l, r) where r = size()
    std::reverse(m_stack.begin() + l, m_stack.end());
    expand();
  }
}
