#pragma once

#include "render/graph_query.hpp"

#include "ecs/exceptions.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
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
  out.bboxValid = false;

  auto *posPool = graph.NodeSpace().template GetPool<PositionComponent>();
  if (!posPool)
    return;

  bool useDefaultRadius = false;
  auto *radiusPool = graph.NodeSpace().template GetPool<RadiusComponent>();
  if (!radiusPool)
    useDefaultRadius = true;

  const VisibleNodesResource *visible =
      graph.template HasResource<VisibleNodesResource>()
          ? graph.template GetResource<VisibleNodesResource>()
          : nullptr;

  out.positions.reserve(graph.Size() * 3);
  out.radii.reserve(graph.Size());

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

  if (anyVisible) {
    std::copy(minB, minB + 3, out.bboxMin);
    std::copy(maxB, maxB + 3, out.bboxMax);
    out.bboxValid = true;
  }

  for (EdgeID eid : graph.Edges()) {
    EdgeComponent e = graph.GetEdge(eid);
    DenseNodeID a = graph.MapToDense(e.from);
    DenseNodeID b = graph.MapToDense(e.to);
    bool aVisible =
        visible ? (a.Raw() < visible->Size() && visible->Test(a)) : true;
    bool bVisible =
        visible ? (b.Raw() < visible->Size() && visible->Test(b)) : true;
    if (aVisible && bVisible) {
      out.edges.push_back(a.Raw());
      out.edges.push_back(b.Raw());
    }
  }
}
