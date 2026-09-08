#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "layout/components/position.hpp"
#include "layout/types.hpp"
#include <cstdint>
#include <random>
#include <sys/types.h>

template <GraphLike G> class PositionRandomized {
public:
  PositionRandomized(G &graph);
  PositionRandomized(G &graph, uint32_t seed);
  ~PositionRandomized() = default;

  void SetBoundingBox(uint32_t width);
  void SetSeed(uint32_t seed);

  void PlaceAll();

private:
  uint32_t m_bboxWidth;
  std::mt19937 m_rng;
  std::uniform_real_distribution<double> m_dist;
  uint8_t m_dimension;
  DenseComponentPool<PositionComponent> *m_positionPool;
  G *m_graph;
};

#include "layout/placement/randomized.tpp"
