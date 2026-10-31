#pragma once

#include "concept/graphLike.hpp"
#include "graph/decorated_graph.hpp"

namespace Decorations {

// checks if the graphlike is a tree, if so it sets:
//  1- TreeNodeDecorator for each node.
//  2- TreeCertificateResource
// Returns false if not a tree.
template <GraphLike G> bool DecorateTree(DecoratedGraph<G> &graph, NodeID root);

} // namespace Decorations

#include "graph/decorations/tree.tpp"
