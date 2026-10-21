#pragma once

#include "graph/types.hpp"
#include <cstdint>

enum class DimensionResource : uint8_t { D2 = 2, D3 = 3 };

enum class EdgeLengthMode : uint8_t { Uniform, DepthBased, SubtreeSize };

using VisibleNodesResource = DenseNodeSet;

struct FixedNodesResource : DenseNodeSet {
  using DenseNodeSet::DenseNodeSet;
};
