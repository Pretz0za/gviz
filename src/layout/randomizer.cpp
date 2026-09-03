#include "layout/randomizer.hpp"
#include "ecs/exceptions.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include <cstdint>
#include <ctime>
#include <random>

#define DEFAULT_BBOX_WIDTH 1000

template <GraphLike G>
PositionRandomizerSystem<G>::PositionRandomizerSystem(G &graph)
    : m_positionPool{&graph.Ecs().template GetPool<PositionComponent>()},
      m_bboxWidth(DEFAULT_BBOX_WIDTH), m_graph(&graph), m_dist{-1, 1},
      m_rng{std::random_device{}()} {
  DimensionResource *dim =
      graph.Ecs().template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = *dim;
}

template <GraphLike G>
void PositionRandomizerSystem<G>::SetSeed(uint32_t seed) {
  m_rng.seed(seed);
};

template <GraphLike G>
void PositionRandomizerSystem<G>::SetBoundingBox(uint32_t width) {
  m_bboxWidth = width;
};

// NOTE: decision: iterate over the vertices in the graph, not the component
// pool. This allows for explicitly defined subgraphs to be worked, instead of
// anything with that Component

template <GraphLike G> void PositionRandomizerSystem<G>::Tick() {
  PositionComponent *c;
  for (NodeID nid : m_graph->Nodes()) {
    c = m_positionPool->Find(nid.Raw());
    if (c == nullptr)
      throw MissingComponentException<PositionComponent>(nid.Raw());
    for (uint8_t d = 0; d < static_cast<uint8_t>(m_dimension); d++)
      c->pos[d] = m_bboxWidth * m_dist(m_rng);
  }
};
