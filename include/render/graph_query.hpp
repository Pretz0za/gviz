#pragma once

#include "concept/graphLike.hpp"
#include "layout/types.hpp"
#include <cstdint>
#include <vector>

struct FrameData {
  std::vector<float> positions;
  std::vector<uint32_t> nodeIds;
  std::vector<uint32_t> edges;
  std::vector<float> radii;

  float bboxMin[3] = {0.0f, 0.0f, 0.0f};
  float bboxMax[3] = {0.0f, 0.0f, 0.0f};
  bool bboxValid = false;
};

template <GraphLike G> bool ReadIs3D(G &graph);

// Reuses out's vectors across calls (clears, doesn't reallocate).
template <GraphLike G>
void ExtractFrameData(G &graph, bool is3D, FrameData &out);

#include "render/graph_query.tpp"
