#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "layout/components/position.hpp"
#include "layout/types.hpp"
#include <cstdint>
#include <random>
#include <sys/types.h>

template <GraphLike G> class PositionRandomizerSystem {
public:
  PositionRandomizerSystem(G &graph);
  PositionRandomizerSystem(G &graph, uint32_t seed);
  ~PositionRandomizerSystem() = default;

  void SetBoundingBox(uint32_t width);
  void SetSeed(uint32_t seed);

  void Tick();

private:
  uint32_t m_bboxWidth;
  std::mt19937 m_rng;
  std::uniform_real_distribution<double> m_dist;
  DimensionResource m_dimension;
  ComponentPool<PositionComponent> *m_positionPool;
  G *m_graph;
};

#include "layout/randomizer.tpp"
