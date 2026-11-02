#pragma once

#include "ecs/components.hpp"
#include "graph/types.hpp"

struct RTNodeDecorator : Component {
  std::vector<double> offsets;

  // left and right most at the last layer of the subtree at this node
  double lMax = 0.0;
  double rMax = 0.0;
  uint32_t depth = 0;
  NodeID lMost = INVALID_NODE_ID;
  NodeID rMost = INVALID_NODE_ID;
  NodeID ancestor = INVALID_NODE_ID;
  NodeID threadTo = INVALID_NODE_ID;
};
