#pragma once

#include "concept/graphLike.hpp"
#include "graph/decorations/tree.hpp"
#include "graph/types.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include "reingold_tilford.hpp"

template <GraphLike G>
ReingoldTilfordLayoutAlgorithm<G>::ReingoldTilfordLayoutAlgorithm(
    G &graph, NodeID root, Configuration *config)
    : m_config(config), m_view(&graph), m_dg(m_view),
      m_decorators(m_dg.nodeSpace.template GetPool<RTNodeDecorator>()) {
  SetRoot(root);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::SetRoot(NodeID id) {
  Decorations::DecorateTree(m_dg, id);
}
