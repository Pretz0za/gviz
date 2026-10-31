#pragma once

#include "ecs/components.hpp"

struct RTNodeDecorator : Component {
  std::vector<float> offsets;

  // left and right most at the last layer of the subtree at this node
  uint32_t lMost = 0;
  uint32_t rMost = 0;
  uint32_t depth = 0;
  uint32_t ancestor = 0;
  uint32_t threadTo = 0;
};
