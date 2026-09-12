#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/components/physics.hpp"
#include "layout/helpers/distance.hpp"
#include <cstdint>

class VanillaKamadaKawai {};

template <GraphLike G> class GRIPKamadaKawai {
public:
  GRIPKamadaKawai(G &graph);

  void Tick(DenseNodeID v, DenseNodeID u, uint32_t graphDist);

private:
  DenseComponentPool<PhysicsComponent> *m_physics;
  DistanceCalculationSystem<G> m_distanceCalc;
  uint8_t m_dimension;
};

#include "layout/physics/kamada_kawai.tpp"
