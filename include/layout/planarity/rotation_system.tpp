#pragma once

#include "graph/components/adjacency.hpp"
#include "layout/planarity/boyer_myrvold.hpp"
#include "layout/planarity/rotation_system.hpp"
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

template <GraphLike G>
PlanarRotationSystem<G>::PlanarRotationSystem(G &graph)
    : m_graph(&graph),
      m_rotation(graph.NodeSpace().template SetPool<PlanarRotationAdjacencyComponent>()) {}

template <GraphLike G> void PlanarRotationSystem<G>::Compute() {
  uint32_t n = m_graph->Size();
  std::vector<std::pair<uint32_t, uint32_t>> edges;

  for (NodeID u : m_graph->Nodes()) {
    uint32_t du = m_graph->MapToDense(u).Raw();
    for (const AdjEntry &adj : m_graph->OutNeighbors(u)) {
      uint32_t dv = m_graph->MapToDense(adj.other).Raw();
      if (dv > du)
        edges.push_back({du, dv});
    }
  }

  std::vector<std::vector<uint32_t>> rotation = ComputePlanarRotation(n, edges);

  for (NodeID u : m_graph->Nodes()) {
    DenseNodeID du = m_graph->MapToDense(u);

    std::unordered_map<uint32_t, AdjEntry> byDenseNeighbor;
    for (const AdjEntry &adj : m_graph->OutNeighbors(u))
      byDenseNeighbor.emplace(m_graph->MapToDense(adj.other).Raw(), adj);

    std::vector<AdjEntry> ordered;
    ordered.reserve(rotation[du.Raw()].size());
    for (uint32_t dv : rotation[du.Raw()])
      ordered.push_back(byDenseNeighbor.at(dv));

    m_rotation->Find(du.Raw())->out = std::move(ordered);
  }
}
