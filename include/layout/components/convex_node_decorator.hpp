#pragma once

#include "ecs/components.hpp"
#include "graph/types.hpp"
#include <cstdint>

enum class TreeShape : uint8_t { Unclassified, Path, Rake, Other };

struct ConvexNodeDecorator : Component {
  TreeShape shape = TreeShape::Unclassified;
  bool chainable = false;
  double firstLeafSlope = 0.0;
  double lastLeafSlope = 0.0;
  NodeID parent = INVALID_NODE_ID;
  uint32_t depth = 0;
  uint32_t subtreeSize = 1;
};
