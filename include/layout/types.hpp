#pragma once

#include "graph/types.hpp"
#include <cstdint>

enum class EdgeLengthMode : uint8_t { Uniform, DepthBased, SubtreeSize };

using VisibleNodesResource = DenseNodeSet;

struct FixedNodesResource : DenseNodeSet {
  using DenseNodeSet::DenseNodeSet;
};
