#pragma once

#include "ecs/exceptions.hpp"
#include "layout/components/position.hpp"
#include "layout/placement/randomized.hpp"
#include <cstdint>
#include <random>

template <GraphLike G>
PositionRandomized<G>::PositionRandomized(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)), m_rng{std::random_device{}()},
      m_dist{-1, 1}, m_dimension(dg.dimension),
      m_positionPool{dg.nodeSpace.template GetPool<PositionComponent>()} {}

template <GraphLike G>
PositionRandomized<G>::PositionRandomized(DecoratedGraph<G> &dg, uint32_t seed, Configuration *config)
    : m_config(RequireConfiguration(config)), m_rng{seed}, m_dist{-1, 1},
      m_dimension(dg.dimension),
      m_positionPool{dg.nodeSpace.template GetPool<PositionComponent>()} {}

template <GraphLike G>
void PositionRandomized<G>::SetSeed(uint32_t seed) {
  m_rng.seed(seed);
};

template <GraphLike G> void PositionRandomized<G>::PlaceAll() {
  for (auto &c : m_positionPool->Data()) {
    for (uint8_t d = 0; d < m_dimension; d++)
      c.pos[d] = m_config->boundingBoxWidth * m_dist(m_rng);
  }
};
