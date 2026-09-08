#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/knearest.hpp"
#include "layout/components/position.hpp"
#include "layout/components/physics.hpp"
#include <cstdint>

class VanillaKamadaKawai {};

template <GraphLike G> class GRIPKamadaKawai {
public:
  GRIPKamadaKawai();

  void Tick(DenseNodeID v, DenseNodeID u, uint32_t graphDist);

private:
  DenseComponentPool<KNearestComponent<32>> *m_knn;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<PhysicsComponent> *m_physics;
};

#include "layout/physics/kamada_kawai.tpp"
