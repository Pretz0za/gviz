#pragma once

#include "graph/components/adjacency.hpp"
#include "layout/planarity/schnyder_layout.hpp"
#include "layout/planarity/triangulation_system.hpp"
#include "layout/types.hpp"
#include <stdexcept>
#include <utility>

template <GraphLike G>
SchnyderLayoutAlgorithm<G>::SchnyderLayoutAlgorithm(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)), m_graph(&graph), m_rotation(graph) {
  m_positions = graph.NodeSpace().template SetPool<PositionComponent>(2);
  graph.template SetResource<DimensionResource>(DimensionResource::D2);
}

template <GraphLike G> void SchnyderLayoutAlgorithm<G>::RebuildWood(NodeID s0, NodeID s1) {
  m_wood = std::make_unique<SchnyderWood<G>>(m_rotation, s0, s1);
  m_root[0] = m_wood->Root(0);
  m_root[1] = m_wood->Root(1);
  m_root[2] = m_wood->Root(2);

  ComputeCoordinates();
}

template <GraphLike G> void SchnyderLayoutAlgorithm<G>::Layout() {
  if (m_graph->Size() < 3)
    throw std::runtime_error("SchnyderLayoutAlgorithm requires at least 3 vertices");

  m_rotation.Compute();
  TriangulationSystem<G> triangulation(m_rotation);
  triangulation.Compute();

  NodeID s0 = m_graph->MapToSparse(DenseNodeID{0});
  NodeID s1 = m_rotation.Neighbors(s0)[0].other;
  RebuildWood(s0, s1);

  m_lastConfigHash = m_config->Hash();
  m_hasRun = true;
}

template <GraphLike G> void SchnyderLayoutAlgorithm<G>::Tick() {
  uint64_t hash = m_config->Hash();
  if (m_hasRun && hash == m_lastConfigHash)
    return;

  if (!m_hasRun) {
    Layout();
    return;
  }

  m_lastConfigHash = hash;
  ComputeCoordinates();
}

template <GraphLike G> void SchnyderLayoutAlgorithm<G>::RandomizeRoots() {
  if (!m_hasRun)
    return;

  uint32_t n = m_graph->Size();
  std::uniform_int_distribution<uint32_t> pickNode(0, n - 1);

  NodeID s0;
  const std::vector<AdjEntry> *out0;
  do {
    s0 = m_graph->MapToSparse(DenseNodeID{pickNode(m_rng)});
    out0 = &m_rotation.Neighbors(s0);
  } while (out0->empty());

  std::uniform_int_distribution<size_t> pickNeighbor(0, out0->size() - 1);
  NodeID s1 = (*out0)[pickNeighbor(m_rng)].other;

  RebuildWood(s0, s1);
}

template <GraphLike G> void SchnyderLayoutAlgorithm<G>::ComputeCoordinates() {
  const SchnyderWood<G> &wood = *m_wood;
  const std::array<NodeID, 3> &root = m_root;

  uint32_t n = m_graph->Size();

  double side = m_config->scale;
  std::array<std::pair<double, double>, 3> corners = {
      std::make_pair(0.0, 0.0),
      std::make_pair(side, 0.0),
      std::make_pair(side * 0.5, side * 0.8660254037844386),
  };

  auto denseOf = [&](NodeID v) { return m_graph->MapToDense(v).Raw(); };

  std::vector<int> pathColor(n, -1);
  std::vector<bool> visited(n, false);
  std::vector<NodeID> touched;
  std::vector<NodeID> queue;
  std::vector<NodeID> visitedList;

  auto &posData = m_positions->Data();

  for (NodeID v : m_graph->Nodes()) {
    if (v == root[0] || v == root[1] || v == root[2])
      continue;

    touched.clear();
    pathColor[denseOf(v)] = 3;
    touched.push_back(v);

    std::array<uint32_t, 3> pathLength = {0, 0, 0};

    for (size_t r = 0; r < 3; r++) {
      NodeID cur = wood.Parent(r, v);
      while (cur.IsValid()) {
        uint32_t dc = denseOf(cur);
        if (pathColor[dc] == -1) {
          pathColor[dc] = static_cast<int>(r);
          touched.push_back(cur);
        }
        pathLength[r]++;
        if (cur == root[r])
          break;
        cur = wood.Parent(r, cur);
      }
    }

    long double counts[3] = {
        static_cast<long double>(pathLength[1]),
        static_cast<long double>(pathLength[2]),
        static_cast<long double>(pathLength[0]),
    };

    visitedList.clear();

    for (NodeID w : m_graph->Nodes()) {
      uint32_t dw = denseOf(w);
      if (pathColor[dw] != -1 || visited[dw])
        continue;

      queue.clear();
      queue.push_back(w);
      visited[dw] = true;
      visitedList.push_back(w);

      bool touchedColor[3] = {false, false, false};
      size_t componentSize = 0;
      size_t head = 0;

      while (head < queue.size()) {
        NodeID cur = queue[head++];
        componentSize++;

        for (const AdjEntry &adj : m_rotation.Neighbors(cur)) {
          uint32_t dn = denseOf(adj.other);
          int color = pathColor[dn];
          if (color != -1) {
            if (color >= 0 && color <= 2)
              touchedColor[color] = true;
            continue;
          }
          if (visited[dn])
            continue;
          visited[dn] = true;
          visitedList.push_back(adj.other);
          queue.push_back(adj.other);
        }
      }

      int region = -1;
      int unTouchedCount = 0;
      for (int c = 0; c < 3; c++) {
        if (!touchedColor[c]) {
          region = c;
          unTouchedCount++;
        }
      }
      if (unTouchedCount != 1)
        throw std::runtime_error("SchnyderLayoutAlgorithm: region boundary is inconsistent");

      counts[region] += static_cast<long double>(componentSize);
    }

    for (NodeID w : visitedList)
      visited[denseOf(w)] = false;

    long double sum = counts[0] + counts[1] + counts[2];
    long double x = (counts[0] * static_cast<long double>(corners[0].first) +
                      counts[1] * static_cast<long double>(corners[1].first) +
                      counts[2] * static_cast<long double>(corners[2].first)) /
                     sum;
    long double y = (counts[0] * static_cast<long double>(corners[0].second) +
                      counts[1] * static_cast<long double>(corners[1].second) +
                      counts[2] * static_cast<long double>(corners[2].second)) /
                     sum;

    posData[denseOf(v)].pos[0] = static_cast<double>(x);
    posData[denseOf(v)].pos[1] = static_cast<double>(y);

    for (NodeID w : touched)
      pathColor[denseOf(w)] = -1;
  }

  for (size_t i = 0; i < 3; i++) {
    uint32_t dr = denseOf(root[i]);
    posData[dr].pos[0] = corners[i].first;
    posData[dr].pos[1] = corners[i].second;
  }
}
