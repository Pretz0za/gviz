#pragma once

#include "graph/components/adjacency.hpp"
#include "layout/algorithms/sankey.hpp"
#include <algorithm>
#include <utility>

template <GraphLike G>
SankeyLayoutAlgorithm<G>::SankeyLayoutAlgorithm(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)), m_graph(&graph) {
  m_info = graph.NodeSpace().template SetPool<SankeyNodeDecorator>();
  m_curves = graph.EdgeSpace().template SetPool<CubicBezierComponent>();
  m_halfWidths = graph.EdgeSpace().template SetPool<HalfWidthComponent>();
  m_positions = graph.NodeSpace().template SetPool<PositionComponent>(2);
  m_weights = graph.EdgeSpace().template GetPool<WeightComponent>();

  graph.template SetResource<DimensionResource>(DimensionResource::D2);
}

template <GraphLike G>
double SankeyLayoutAlgorithm<G>::EdgeWeight(EdgeID id) const {
  if (!m_weights)
    return 1.0;
  const WeightComponent *w = m_weights->Find(m_graph->MapToDense(id).Raw());
  return w ? static_cast<double>(w->value) : 1.0;
}

template <GraphLike G> void SankeyLayoutAlgorithm<G>::AssignLayers() {
  uint32_t n = m_graph->Size();
  std::vector<uint32_t> indegree(n, 0);
  for (uint32_t i = 0; i < n; i++)
    indegree[i] = m_graph->InDegree(m_graph->MapToSparse(DenseNodeID{i}));

  std::vector<DenseNodeID> queue;
  for (uint32_t i = 0; i < n; i++)
    if (indegree[i] == 0)
      queue.push_back(DenseNodeID{i});

  std::vector<uint32_t> layer(n, 0);
  size_t qi = 0;
  uint32_t maxLayer = 0;
  while (qi < queue.size()) {
    DenseNodeID v = queue[qi++];
    for (const AdjEntry &adj : m_graph->OutNeighbors(m_graph->MapToSparse(v))) {
      DenseNodeID w = m_graph->MapToDense(adj.other);
      layer[w.Raw()] = std::max(layer[w.Raw()], layer[v.Raw()] + 1);
      maxLayer = std::max(maxLayer, layer[w.Raw()]);
      if (--indegree[w.Raw()] == 0)
        queue.push_back(w);
    }
  }

  m_layerCount = maxLayer + 1;
  m_layers.assign(m_layerCount, {});
  for (uint32_t i = 0; i < n; i++) {
    m_info->Find(i)->layer = layer[i];
    m_layers[layer[i]].push_back(DenseNodeID{i});
  }
}

template <GraphLike G> void SankeyLayoutAlgorithm<G>::OrderWithinLayers() {
  for (auto &nodes : m_layers)
    std::ranges::sort(nodes, {}, [](DenseNodeID id) { return id.Raw(); });

  for (uint32_t i = 0; i < m_layerCount; i++)
    for (uint32_t pos = 0; pos < m_layers[i].size(); pos++)
      m_info->Find(m_layers[i][pos].Raw())->order = pos;

  for (uint32_t i = 1; i < m_layerCount; i++) {
    std::vector<std::pair<double, DenseNodeID>> keyed;
    keyed.reserve(m_layers[i].size());
    for (DenseNodeID v : m_layers[i]) {
      double sum = 0.0;
      uint32_t count = 0;
      for (const AdjEntry &adj : m_graph->InNeighbors(m_graph->MapToSparse(v))) {
        sum += static_cast<double>(
            m_info->Find(m_graph->MapToDense(adj.other).Raw())->order);
        count++;
      }
      double key = count > 0 ? sum / static_cast<double>(count)
                             : static_cast<double>(m_info->Find(v.Raw())->order);
      keyed.push_back({key, v});
    }
    std::ranges::sort(keyed, {}, [](const auto &p) { return p.first; });
    for (uint32_t pos = 0; pos < keyed.size(); pos++) {
      m_layers[i][pos] = keyed[pos].second;
      m_info->Find(keyed[pos].second.Raw())->order = pos;
    }
  }
}

template <GraphLike G> void SankeyLayoutAlgorithm<G>::ComputeExtents() {
  uint32_t n = m_graph->Size();
  for (uint32_t i = 0; i < n; i++) {
    NodeID sparse = m_graph->MapToSparse(DenseNodeID{i});
    double inSum = 0.0;
    for (const AdjEntry &adj : m_graph->InNeighbors(sparse))
      inSum += EdgeWeight(adj.edge);
    double outSum = 0.0;
    for (const AdjEntry &adj : m_graph->OutNeighbors(sparse))
      outSum += EdgeWeight(adj.edge);
    double flow = std::max(inSum, outSum);
    m_info->Find(i)->extent =
        std::max(flow * m_config->thicknessScale, m_config->minThickness);
  }
}

template <GraphLike G> void SankeyLayoutAlgorithm<G>::PlaceNodes() {
  m_columnX.assign(m_layerCount, 0.0);
  for (uint32_t i = 0; i < m_layerCount; i++)
    m_columnX[i] = static_cast<double>(i) * m_config->columnSpacing;

  auto &pos = m_positions->Data();
  for (uint32_t i = 0; i < m_layerCount; i++) {
    double y = 0.0;
    for (DenseNodeID v : m_layers[i]) {
      SankeyNodeDecorator *info = m_info->Find(v.Raw());
      double center = y + info->extent / 2.0;
      pos[v.Raw()].pos[0] = m_columnX[i];
      pos[v.Raw()].pos[1] = center;
      info->inCursor = y;
      info->outCursor = y;
      y += info->extent + m_config->nodeSpacing;
    }
  }
}

template <GraphLike G>
void SankeyLayoutAlgorithm<G>::AssignPortsAndBuildCurves() {
  auto &pos = m_positions->Data();

  for (uint32_t layerIdx = 0; layerIdx < m_layerCount; layerIdx++) {
    for (DenseNodeID v : m_layers[layerIdx]) {
      NodeID sparse = m_graph->MapToSparse(v);
      std::vector<AdjEntry> out(m_graph->OutNeighbors(sparse).begin(),
                                m_graph->OutNeighbors(sparse).end());
      std::ranges::sort(out, {}, [this](const AdjEntry &adj) {
        return m_info->Find(m_graph->MapToDense(adj.other).Raw())->order;
      });

      for (const AdjEntry &adj : out) {
        SankeyNodeDecorator *srcInfo = m_info->Find(v.Raw());
        DenseNodeID target = m_graph->MapToDense(adj.other);
        SankeyNodeDecorator *dstInfo = m_info->Find(target.Raw());

        double weight = EdgeWeight(adj.edge) * m_config->thicknessScale;
        double halfWidth =
            std::max(weight / 2.0, m_config->minThickness / 2.0);

        double sy = srcInfo->outCursor + halfWidth;
        srcInfo->outCursor += weight;

        double dy = dstInfo->inCursor + halfWidth;
        dstInfo->inCursor += weight;

        double sx = pos[v.Raw()].pos[0] + m_config->nodeWidth / 2.0;
        double dx = pos[target.Raw()].pos[0] - m_config->nodeWidth / 2.0;
        double dxCtrl = (dx - sx) * m_config->curvature;

        DenseEdgeID edge = m_graph->MapToDense(adj.edge);
        CubicBezierComponent *curve = m_curves->Find(edge.Raw());
        curve->p0[0] = sx;
        curve->p0[1] = sy;
        curve->p1[0] = sx + dxCtrl;
        curve->p1[1] = sy;
        curve->p2[0] = dx - dxCtrl;
        curve->p2[1] = dy;
        curve->p3[0] = dx;
        curve->p3[1] = dy;
        m_halfWidths->Find(edge.Raw())->halfWidth = halfWidth;
      }
    }
  }
}

template <GraphLike G> void SankeyLayoutAlgorithm<G>::Layout() {
  if (m_graph->Size() == 0)
    return;

  AssignLayers();
  OrderWithinLayers();
  ComputeExtents();
  PlaceNodes();
  AssignPortsAndBuildCurves();
}

template <GraphLike G> void SankeyLayoutAlgorithm<G>::Tick() {
  uint64_t hash = m_config->Hash();
  if (m_hasRun && hash == m_lastConfigHash)
    return;

  m_lastConfigHash = hash;
  m_hasRun = true;
  Layout();
}
