#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace Decorations::Detail {

struct PlanarEmbeddingResult {
  bool planar = false;
  std::vector<std::vector<uint32_t>> rotation;
  std::vector<uint32_t> kuratowskiVertices;
};

PlanarEmbeddingResult
ComputePlanarEmbedding(uint32_t vertexCount,
                       const std::vector<std::pair<uint32_t, uint32_t>> &edges);

} // namespace Decorations::Detail
