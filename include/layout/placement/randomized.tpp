#pragma once

#include "ecs/exceptions.hpp"
#include "layout/components/position.hpp"
#include "layout/placement/randomized.hpp"
#include <cstdint>
#include <random>

template <GraphLike G>
PositionRandomized<G>::PositionRandomized(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_positionPool{graph.NodeSpace().template GetPool<PositionComponent>()},
      m_graph(&graph), m_dist{-1, 1}, m_rng{std::random_device{}()} {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G>
PositionRandomized<G>::PositionRandomized(G &graph, uint32_t seed, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_positionPool{graph.NodeSpace().template GetPool<PositionComponent>()},
      m_graph(&graph), m_dist{-1, 1}, m_rng{seed} {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);
}

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
