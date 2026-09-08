#pragma once

#include "concept/graphLike.hpp"
#include "graph/types.hpp"
#include "layout/physics/fruchterman_reingold.hpp"

template <GraphLike G> GRIPFruchtermanReingold<G>::GRIPFruchtermanReingold() {}

template <GraphLike G>
void GRIPFruchtermanReingold<G>::Tick(DenseNodeID v, DenseNodeID u) {}
