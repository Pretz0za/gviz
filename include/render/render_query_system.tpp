#pragma once

#include "render/render_query_system.hpp"

#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include <algorithm>
#include <limits>

template <GraphLike G> bool ReadIs3D(DecoratedGraph<G> &dg) {
  return dg.dimension == 3;
}

template <GraphLike G>
RenderQuerySystem<G>::RenderQuerySystem(DecoratedGraph<G> &dg)
    : m_graph(dg.graph),
      m_position(dg.nodeSpace.template GetPool<PositionComponent>()),
      m_radius(dg.nodeSpace.template GetPool<RadiusComponent>()),
      m_nodeColor(dg.nodeSpace.template GetPool<ColorComponent>()),
      m_edgeColor(dg.edgeSpace.template GetPool<ColorComponent>()),
      m_bezier(dg.edgeSpace.template GetPool<CubicBezierComponent>()),
      m_halfWidth(dg.edgeSpace.template GetPool<HalfWidthComponent>()),
      m_visible(dg.template HasResource<VisibleNodesResource>()
                    ? dg.template GetResource<VisibleNodesResource>()
                    : nullptr) {}

template <GraphLike G>
bool RenderQuerySystem<G>::IsNodeVisible(DenseNodeID id) const {
  return m_visible ? (id.Raw() < m_visible->Size() && m_visible->Test(id))
                   : true;
}

template <GraphLike G>
float RenderQuerySystem<G>::HalfWidthAt(DenseEdgeID deid) const {
  if (!m_halfWidth || deid.Raw() >= m_halfWidth->Size())
    return 0.0f;
  float hw = static_cast<float>(m_halfWidth->Find(deid.Raw())->halfWidth);
  return hw > 0.0f ? hw : 0.0f;
}

template <GraphLike G>
void RenderQuerySystem<G>::ExtractFrameData(bool is3D, FrameData &out) {
  out.positions.clear();
  out.nodeIds.clear();
  out.edges.clear();
  out.radii.clear();
  out.colors.clear();
  out.edgeColors.clear();
  out.bezierControlPoints.clear();
  out.halfWidths.clear();
  out.nodeRectHalfHeights.clear();
  out.bboxValid = false;

  if (!m_position)
    return;

  float minB[3] = {std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()};
  float maxB[3] = {std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest()};
  bool anyVisible = false;

  ExtractNodes(is3D, out, minB, maxB, anyVisible);
  ExtractNodeRectHalfHeights(out);
  ExtractEdges(out, minB, maxB, anyVisible);

  if (anyVisible) {
    std::copy(minB, minB + 3, out.bboxMin);
    std::copy(maxB, maxB + 3, out.bboxMax);
    out.bboxValid = true;
  }
}

template <GraphLike G>
void RenderQuerySystem<G>::ExtractNodes(bool is3D, FrameData &out,
                                        float minB[3], float maxB[3],
                                        bool &anyVisible) {
  out.positions.reserve(m_graph->Size() * 3);
  out.radii.reserve(m_graph->Size());
  out.colors.reserve(m_graph->Size());

  for (uint32_t i = 0; i < m_graph->Size(); i++) {
    DenseNodeID denseID{i};

    const PositionComponent *c = m_position->Find(denseID.Raw());
    float x = c ? static_cast<float>(c->pos[0]) : 0.0f;
    float y = c ? static_cast<float>(c->pos[1]) : 0.0f;
    float z = (c && is3D) ? static_cast<float>(c->pos[2]) : 0.0f;
    out.positions.push_back(x);
    out.positions.push_back(y);
    out.positions.push_back(z);

    out.radii.push_back(
        m_radius ? static_cast<float>(m_radius->Find(denseID.Raw())->radius)
                 : DEFAULT_RADIUS);

    out.colors.push_back(m_nodeColor
                             ? m_nodeColor->Find(denseID.Raw())->color
                             : DEFAULT_COLOR);

    if (IsNodeVisible(denseID)) {
      out.nodeIds.push_back(denseID.Raw());
      anyVisible = true;
      minB[0] = std::min(minB[0], x);
      minB[1] = std::min(minB[1], y);
      minB[2] = std::min(minB[2], z);
      maxB[0] = std::max(maxB[0], x);
      maxB[1] = std::max(maxB[1], y);
      maxB[2] = std::max(maxB[2], z);
    }
  }
}

template <GraphLike G>
void RenderQuerySystem<G>::ExtractNodeRectHalfHeights(FrameData &out) {
  if (!m_halfWidth)
    return;

  out.nodeRectHalfHeights.reserve(m_graph->Size());

  for (uint32_t i = 0; i < m_graph->Size(); i++) {
    DenseNodeID denseID{i};
    NodeID sparse = m_graph->MapToSparse(denseID);

    float outSum = 0.0f;
    for (const AdjEntry &adj : m_graph->OutNeighbors(sparse))
      outSum += HalfWidthAt(m_graph->MapToDense(adj.edge));
    float inSum = 0.0f;
    for (const AdjEntry &adj : m_graph->InNeighbors(sparse))
      inSum += HalfWidthAt(m_graph->MapToDense(adj.edge));

    out.nodeRectHalfHeights.push_back(std::max(outSum, inSum));
  }
}

template <GraphLike G>
void RenderQuerySystem<G>::ExtractEdges(FrameData &out, float minB[3],
                                        float maxB[3], bool &anyVisible) {
  if (m_edgeColor)
    out.edgeColors.reserve(m_edgeColor->Size());

  for (EdgeID eid : m_graph->Edges()) {
    EdgeComponent e = m_graph->GetEdge(eid);
    DenseNodeID a = m_graph->MapToDense(e.from);
    DenseNodeID b = m_graph->MapToDense(e.to);
    if (!IsNodeVisible(a) || !IsNodeVisible(b))
      continue;

    DenseEdgeID deid = m_graph->MapToDense(eid);
    const CubicBezierComponent *curve =
        (m_bezier && deid.Raw() < m_bezier->Size()) ? m_bezier->Find(deid.Raw())
                                                    : nullptr;

    if (curve && IsCurveSet(*curve))
      ExtractBezierEdge(deid, *curve, out, minB, maxB, anyVisible);
    else
      ExtractStraightEdge(a, b, deid, out);
  }
}

template <GraphLike G>
bool RenderQuerySystem<G>::IsCurveSet(const CubicBezierComponent &curve) const {
  return curve.p0[0] != 0.0 || curve.p0[1] != 0.0 || curve.p1[0] != 0.0 ||
        curve.p1[1] != 0.0 || curve.p2[0] != 0.0 || curve.p2[1] != 0.0 ||
        curve.p3[0] != 0.0 || curve.p3[1] != 0.0;
}

template <GraphLike G>
void RenderQuerySystem<G>::ExtractBezierEdge(DenseEdgeID deid,
                                             const CubicBezierComponent &curve,
                                             FrameData &out, float minB[3],
                                             float maxB[3], bool &anyVisible) {
  uint32_t color = (m_edgeColor && deid.Raw() < m_edgeColor->Size())
                       ? m_edgeColor->Find(deid.Raw())->color
                       : DEFAULT_COLOR;

  float halfWidth = HalfWidthAt(deid);
  if (halfWidth <= 0.0f)
    halfWidth = static_cast<float>(DEFAULT_HALF_WIDTH);

  const double *controlPoints[4] = {curve.p0, curve.p1, curve.p2, curve.p3};
  for (const double *cp : controlPoints) {
    float x = static_cast<float>(cp[0]);
    float y = static_cast<float>(cp[1]);
    out.bezierControlPoints.push_back(x);
    out.bezierControlPoints.push_back(y);
    out.bezierControlPoints.push_back(0.0f);

    anyVisible = true;
    minB[0] = std::min(minB[0], x - halfWidth);
    minB[1] = std::min(minB[1], y - halfWidth);
    minB[2] = std::min(minB[2], 0.0f);
    maxB[0] = std::max(maxB[0], x + halfWidth);
    maxB[1] = std::max(maxB[1], y + halfWidth);
    maxB[2] = std::max(maxB[2], 0.0f);
  }
  out.halfWidths.push_back(halfWidth);
  out.edgeColors.push_back(color);
}

template <GraphLike G>
void RenderQuerySystem<G>::ExtractStraightEdge(DenseNodeID a, DenseNodeID b,
                                               DenseEdgeID deid,
                                               FrameData &out) {
  out.edges.push_back(a.Raw());
  out.edges.push_back(b.Raw());

  if (m_edgeColor) {
    out.edgeColors.push_back(deid.Raw() < m_edgeColor->Size()
                                 ? m_edgeColor->Find(deid.Raw())->color
                                 : DEFAULT_COLOR);
  }

  out.halfWidths.push_back(HalfWidthAt(deid));
}
