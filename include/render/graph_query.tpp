#pragma once

#include "render/graph_query.hpp"

#include "ecs/exceptions.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"
#include "layout/components/cubic_bezier.hpp"
#include "layout/components/half_width.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "render/components/color.hpp"
#include <algorithm>
#include <limits>

template <GraphLike G> bool ReadIs3D(G &graph) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  return *dim == DimensionResource::D3;
}

template <GraphLike G>
void ExtractFrameData(G &graph, bool is3D, FrameData &out) {
  out.positions.clear();
  out.nodeIds.clear();
  out.edges.clear();
  out.radii.clear();
  out.colors.clear();
  out.edgeColors.clear();
  out.bezierControlPoints.clear();
  out.bezierHalfWidths.clear();
  out.bezierColors.clear();
  out.nodeRectHalfHeights.clear();
  out.bboxValid = false;

  auto *posPool = graph.NodeSpace().template GetPool<PositionComponent>();
  if (!posPool)
    return;

  bool useDefaultRadius = false;
  auto *radiusPool = graph.NodeSpace().template GetPool<RadiusComponent>();
  if (!radiusPool)
    useDefaultRadius = true;

  bool useDefaultColor = false;
  auto *colorPool = graph.NodeSpace().template GetPool<ColorComponent>();
  if (!colorPool)
    useDefaultColor = true;

  auto *edgeColorPool = graph.EdgeSpace().template GetPool<ColorComponent>();
  if (edgeColorPool)
    out.edgeColors.reserve(edgeColorPool->Size());

  auto *bezierPool = graph.EdgeSpace().template GetPool<CubicBezierComponent>();
  auto *halfWidthPool = graph.EdgeSpace().template GetPool<HalfWidthComponent>();

  // A node's rect half-height (if any) is derived, not stored: the thicker
  // of its total incoming vs. outgoing edge thickness.
  auto edgeHalfWidth = [&](EdgeID eid) -> float {
    if (!halfWidthPool)
      return 0.0f;
    DenseEdgeID deid = graph.MapToDense(eid);
    if (deid.Raw() >= halfWidthPool->Size())
      return 0.0f;
    float hw = static_cast<float>(halfWidthPool->Find(deid.Raw())->halfWidth);
    return hw > 0.0f ? hw : 0.0f;
  };

  const VisibleNodesResource *visible =
      graph.template HasResource<VisibleNodesResource>()
          ? graph.template GetResource<VisibleNodesResource>()
          : nullptr;

  out.positions.reserve(graph.Size() * 3);
  out.radii.reserve(graph.Size());
  out.colors.reserve(graph.Size());
  out.nodeRectHalfHeights.reserve(graph.Size());

  float minB[3] = {std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()};
  float maxB[3] = {std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest()};
  bool anyVisible = false;

  for (uint32_t i = 0; i < graph.Size(); i++) {
    DenseNodeID denseID{i};

    const PositionComponent *c = posPool->Find(denseID.Raw());
    float x = c ? static_cast<float>(c->pos[0]) : 0.0f;
    float y = c ? static_cast<float>(c->pos[1]) : 0.0f;
    float z = (c && is3D) ? static_cast<float>(c->pos[2]) : 0.0f;
    out.positions.push_back(x);
    out.positions.push_back(y);
    out.positions.push_back(z);

    out.radii.push_back(
        useDefaultRadius
            ? DEFAULT_RADIUS
            : static_cast<float>(radiusPool->Find(denseID.Raw())->radius));

    out.colors.push_back(useDefaultColor
                              ? DEFAULT_COLOR
                              : colorPool->Find(denseID.Raw())->color);

    NodeID sparse = graph.MapToSparse(denseID);
    float outSum = 0.0f;
    for (const AdjEntry &adj : graph.OutNeighbors(sparse))
      outSum += edgeHalfWidth(adj.edge);
    float inSum = 0.0f;
    for (const AdjEntry &adj : graph.InNeighbors(sparse))
      inSum += edgeHalfWidth(adj.edge);
    out.nodeRectHalfHeights.push_back(std::max(outSum, inSum));

    bool isVisible =
        visible ? (denseID.Raw() < visible->Size() && visible->Test(denseID))
                : true;
    if (isVisible) {
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

  for (EdgeID eid : graph.Edges()) {
    EdgeComponent e = graph.GetEdge(eid);
    DenseNodeID a = graph.MapToDense(e.from);
    DenseNodeID b = graph.MapToDense(e.to);
    bool aVisible =
        visible ? (a.Raw() < visible->Size() && visible->Test(a)) : true;
    bool bVisible =
        visible ? (b.Raw() < visible->Size() && visible->Test(b)) : true;
    if (!aVisible || !bVisible)
      continue;

    DenseEdgeID deid = graph.MapToDense(eid);
    const CubicBezierComponent *curve =
        (bezierPool && deid.Raw() < bezierPool->Size())
            ? bezierPool->Find(deid.Raw())
            : nullptr;
    const HalfWidthComponent *halfWidthComp =
        (halfWidthPool && deid.Raw() < halfWidthPool->Size())
            ? halfWidthPool->Find(deid.Raw())
            : nullptr;

    if (curve && halfWidthComp && halfWidthComp->halfWidth > 0.0) {
      uint32_t color = (edgeColorPool && deid.Raw() < edgeColorPool->Size())
                            ? edgeColorPool->Find(deid.Raw())->color
                            : DEFAULT_COLOR;
      float halfWidth = static_cast<float>(halfWidthComp->halfWidth);
      const double *controlPoints[4] = {curve->p0, curve->p1, curve->p2,
                                        curve->p3};
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
      out.bezierHalfWidths.push_back(halfWidth);
      out.bezierColors.push_back(color);
      continue;
    }

    out.edges.push_back(a.Raw());
    out.edges.push_back(b.Raw());
    if (edgeColorPool) {
      out.edgeColors.push_back(deid.Raw() < edgeColorPool->Size()
                                    ? edgeColorPool->Find(deid.Raw())->color
                                    : DEFAULT_COLOR);
    }
  }

  if (anyVisible) {
    std::copy(minB, minB + 3, out.bboxMin);
    std::copy(maxB, maxB + 3, out.bboxMax);
    out.bboxValid = true;
  }
}
