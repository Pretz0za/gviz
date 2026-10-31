#pragma once

#include "concept/graphLike.hpp"
#include "graph/decorated_graph.hpp"
#include <stdexcept>

namespace Decorations {

class NotPlanarException : public std::runtime_error {
public:
  NotPlanarException() : std::runtime_error("graph is not planar") {}
};

// checks if the graphlike is planar, if so it sets:
//  1- PlanarRotationAdjacencyComponent (rotation order) for each node.
// Otherwise it sets NonPlanarCertificateResource.
// Returns false if not planar.
template <GraphLike G> bool DecoratePlanar(DecoratedGraph<G> &graph);

} // namespace Decorations

#include "graph/decorations/planar.tpp"
