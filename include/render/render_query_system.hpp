#pragma once

#include "concept/graphLike.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/types.hpp"
#include "layout/components/cubic_bezier.hpp"
#include "layout/components/half_width.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "layout/types.hpp"
#include "render/components/color.hpp"
#include <cstdint>
#include <vector>

struct FrameData {
  std::vector<float> positions;     // required
  std::vector<uint32_t> nodeIds;    // required
  std::vector<uint32_t> edges;      // required
  std::vector<float> radii;         // required
  std::vector<uint32_t> colors;     // required
  std::vector<uint32_t> edgeColors; // required

  std::vector<float> bezierControlPoints; // required
  std::vector<float> halfWidths;

  std::vector<float> nodeRectHalfHeights;

  float bboxMin[3] = {0.0f, 0.0f, 0.0f};
  float bboxMax[3] = {0.0f, 0.0f, 0.0f};
  bool bboxValid = false;
};

template <GraphLike G> bool ReadIs3D(DecoratedGraph<G> &dg);

template <GraphLike G> class RenderQuerySystem {
public:
  explicit RenderQuerySystem(DecoratedGraph<G> &dg);

  // Reuses out's vectors across calls (clears, doesn't reallocate).
  void ExtractFrameData(bool is3D, FrameData &out);

private:
  void ExtractNodes(bool is3D, FrameData &out, float minB[3], float maxB[3],
                     bool &anyVisible);
  void ExtractNodeRectHalfHeights(FrameData &out);
  void ExtractEdges(FrameData &out, float minB[3], float maxB[3],
                     bool &anyVisible);
  void ExtractBezierEdge(DenseEdgeID deid, const CubicBezierComponent &curve,
                         FrameData &out, float minB[3], float maxB[3],
                         bool &anyVisible);
  void ExtractStraightEdge(DenseNodeID a, DenseNodeID b, DenseEdgeID deid,
                           FrameData &out);

  bool IsNodeVisible(DenseNodeID id) const;
  bool IsCurveSet(const CubicBezierComponent &curve) const;
  float HalfWidthAt(DenseEdgeID deid) const;

  G *m_graph;
  DenseComponentPool<PositionComponent> *m_position;
  DenseComponentPool<RadiusComponent> *m_radius;
  DenseComponentPool<ColorComponent> *m_nodeColor;
  DenseComponentPool<ColorComponent> *m_edgeColor;
  DenseComponentPool<CubicBezierComponent> *m_bezier;
  DenseComponentPool<HalfWidthComponent> *m_halfWidth;
  const VisibleNodesResource *m_visible;
};

#include "render/render_query_system.tpp"
