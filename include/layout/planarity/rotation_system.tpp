#pragma once

#include "graph/components/adjacency.hpp"
#include "layout/planarity/boyer_myrvold.hpp"
#include "layout/planarity/rotation_system.hpp"
#include <cstdint>
#include <stdexcept>
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

template <GraphLike G> const std::vector<AdjEntry> &PlanarRotationSystem<G>::Neighbors(NodeID v) const {
  return m_rotation->Find(m_graph->MapToDense(v).Raw())->out;
}

template <GraphLike G> size_t PlanarRotationSystem<G>::IndexOf(NodeID v, NodeID target) const {
  const std::vector<AdjEntry> &out = Neighbors(v);
  for (size_t idx = 0; idx < out.size(); idx++)
    if (out[idx].other == target)
      return idx;
  throw std::runtime_error("vertex is not a rotation neighbor of the given vertex");
}

template <GraphLike G> bool PlanarRotationSystem<G>::HasEdge(NodeID u, NodeID v) const {
  for (const AdjEntry &e : Neighbors(u))
    if (e.other == v)
      return true;
  return false;
}

template <GraphLike G> NodeID PlanarRotationSystem<G>::FaceThirdVertex(NodeID u, NodeID v) const {
  const std::vector<AdjEntry> &outV = Neighbors(v);
  size_t idx = IndexOf(v, u);
  size_t prev = (idx == 0 ? outV.size() : idx) - 1;
  return outV[prev].other;
}

template <GraphLike G> std::vector<std::vector<NodeID>> PlanarRotationSystem<G>::EnumerateFaces() const {
  uint32_t n = m_graph->Size();

  std::vector<std::vector<bool>> visited(n);
  for (NodeID u : m_graph->Nodes())
    visited[m_graph->MapToDense(u).Raw()].assign(Neighbors(u).size(), false);

  std::vector<std::vector<NodeID>> faces;
  for (NodeID start : m_graph->Nodes()) {
    uint32_t dStart = m_graph->MapToDense(start).Raw();
    size_t degree = Neighbors(start).size();

    for (size_t startIdx = 0; startIdx < degree; startIdx++) {
      if (visited[dStart][startIdx])
        continue;

      std::vector<NodeID> face;
      NodeID u = start;
      size_t idx = startIdx;
      while (true) {
        uint32_t du = m_graph->MapToDense(u).Raw();
        if (visited[du][idx])
          break;
        visited[du][idx] = true;

        NodeID v = Neighbors(u)[idx].other;
        face.push_back(v);

        size_t idxOfU = IndexOf(v, u);
        const std::vector<AdjEntry> &outV = Neighbors(v);
        size_t prevIdx = (idxOfU == 0 ? outV.size() : idxOfU) - 1;

        u = v;
        idx = prevIdx;
      }

      faces.push_back(std::move(face));
    }
  }

  return faces;
}

template <GraphLike G> void PlanarRotationSystem<G>::InsertNeighborAt(NodeID v, size_t index, NodeID neighbor) {
  std::vector<AdjEntry> &out = m_rotation->Find(m_graph->MapToDense(v).Raw())->out;
  out.insert(out.begin() + static_cast<ptrdiff_t>(index), AdjEntry{INVALID_EDGE_ID, neighbor});
}
