#pragma once

#include "ecs/components.hpp"
#include "graph/types.hpp"

struct RTNodeDecorator : Component {
  std::vector<float> offsets;

  // left and right most at the last layer of the subtree at this node
  uint32_t depth = 0;
  NodeID lMost = INVALID_NODE_ID;
  NodeID rMost = INVALID_NODE_ID;
  NodeID ancestor = INVALID_NODE_ID;
  NodeID threadTo = INVALID_NODE_ID;
};
