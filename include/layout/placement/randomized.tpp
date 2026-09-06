#pragma once

#include "ecs/exceptions.hpp"
#include "layout/components/position.hpp"
#include "layout/placement/randomized.hpp"
#include <cstdint>
#include <ctime>
#include <random>

#define DEFAULT_BBOX_WIDTH 1000

template <GraphLike G>
PositionRandomized<G>::PositionRandomized(G &graph)
    : m_positionPool{&graph.NodeSpace().template GetPool<PositionComponent>()},
      m_bboxWidth(DEFAULT_BBOX_WIDTH), m_graph(&graph), m_dist{-1, 1},
      m_rng{std::random_device{}()} {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = *dim;
}

template <GraphLike G>
void PositionRandomized<G>::SetSeed(uint32_t seed) {
  m_rng.seed(seed);
};

template <GraphLike G>
void PositionRandomized<G>::SetBoundingBox(uint32_t width) {
  m_bboxWidth = width;
};

template <GraphLike G> void PositionRandomized<G>::PlaceAll() {
  for (auto &c : m_positionPool->Data()) {
    for (uint8_t d = 0; d < static_cast<uint8_t>(m_dimension); d++)
      c.pos[d] = m_bboxWidth * m_dist(m_rng);
  }
};
