#pragma once

#include "graph/types.hpp"
#include <cstdint>

enum class DimensionResource : uint8_t { D2 = 2, D3 = 3 };

// Edge-length functions from arxiv.org/abs/cs/0607113: lengths may be chosen
// as a function of the position of the edge in the tree rather than fixed.
// DepthBased varies length with distance from the root; SubtreeSize varies
// it with the size of the subtree rooted at the edge's top (parent) vertex.
enum class EdgeLengthMode : uint8_t { Uniform, DepthBased, SubtreeSize };

using VisibleNodesResource = DenseNodeSet;
