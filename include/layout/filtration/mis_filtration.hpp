#pragma once

#include "concept/graphLike.hpp"
#include "layout/filtration/types.hpp"

class Bitset;

template <GraphLike G> class MisFiltrationSystem {
public:
  explicit MisFiltrationSystem(G &graph);
  void Tick(NestedFiltrationResult &out);

private:
  void BuildFiltration(NestedFiltrationResult &out);
  void BuildFirstLayer(NestedFiltrationResult &out, Bitset &vertices);
  G *m_graph;
};

#include "layout/filtration/mis_filtration.tpp"
