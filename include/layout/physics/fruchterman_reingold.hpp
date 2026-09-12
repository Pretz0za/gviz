#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/knearest.hpp"
class VanillaFruchtermanReingold {};

template <GraphLike G> class GRIPFruchtermanReingold {
public:
  GRIPFruchtermanReingold();

  void Tick(DenseNodeID v, DenseNodeID u);

private:
  DenseComponentPool<KNearestComponent> *m_knn;
};

#include "layout/physics/fruchterman_reingold.tpp"
