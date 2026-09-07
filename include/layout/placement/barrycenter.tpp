#pragma once

#include "concept/graphLike.hpp"
#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/search/types.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/placement/barrycenter.hpp"
#include "layout/types.hpp"
#include <vector>

template <GraphLike G>
PositionBarrycentric<G>::PositionBarrycentric(G &graph)
    : m_graph(&graph), m_knnSystem(graph) {
  m_positions = &m_graph->NodeSpace().template GetPool<PositionComponent>();
  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = *dim;
}

template <GraphLike G>
void PositionBarrycentric<G>::PlaceBetweenNeighbors(NodeID id) {
  PositionComponent acc{{}, {0, 0, 0}};
  uint32_t count = 0;
  for (const AdjEntry &adj : m_graph->OutNeighbors(id)) {
    auto nbrCompact = m_graph->MapToDense(adj.other);
    PositionComponent *nbrPos = m_positions->Find(nbrCompact.Raw());

    // TODO: perhaps gate this check behind a debug build
    if (nbrPos == nullptr)
      throw MissingComponentException<PositionComponent>(adj.other.Raw());

    count++;
    for (size_t i = 0; i < static_cast<uint8_t>(m_dimension); i++)
      acc.pos[i] += nbrPos->pos[i];
  }

  for (size_t i = 0; i < static_cast<uint8_t>(m_dimension); i++)
    acc.pos[i] /= static_cast<double>(count);

  m_positions->Set(m_graph->MapToDense(id).Raw(), acc);
}

template <GraphLike G>
void PositionBarrycentric<G>::PlaceBetweenKNearest(NodeID id, uint32_t k) {

  std::vector<FoundNode> knearest = m_knnSystem.Find(id, k);
  placeBetweenFound(id, knearest);
}

template <GraphLike G>
void PositionBarrycentric<G>::PlaceBetweenKNearest(NodeID id, uint32_t k,
                                                   DenseNodeSet &filter) {
  std::vector<FoundNode> knearest = m_knnSystem.Find(id, k, filter);
  placeBetweenFound(id, knearest);
}

template <GraphLike G>
void PositionBarrycentric<G>::placeBetweenFound(NodeID id,
                                                std::vector<FoundNode> found) {
  PositionComponent acc{{}, {0, 0, 0}};
  for (const FoundNode &nd : found) {
    auto knnCompact = m_graph->MapToDense(nd.node);
    PositionComponent *knnPos = m_positions->Find(knnCompact.Raw());

    if (knnPos == nullptr)
      throw MissingComponentException<PositionComponent>(nd.node.Raw());

    for (size_t i = 0; i < static_cast<uint8_t>(m_dimension); i++)
      acc.pos[i] += (knnPos->pos[i] / static_cast<double>(found.size()));
  }

  m_positions->Set(m_graph->MapToDense(id).Raw(), acc);
}
