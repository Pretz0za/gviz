#pragma once

#include "concept/graphLike.hpp"

template <GraphLike G> class UndirectedView {
public:
  UndirectedView<G>(G *graph);

private:
  G *m_graph;
};
