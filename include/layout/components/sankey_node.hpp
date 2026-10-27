#pragma once

#include "ecs/components.hpp"
#include <cstdint>

struct SankeyNodeDecorator : Component {
  uint32_t layer = 0;
  uint32_t order = 0;
  double extent = 0.0;
  double inHalf = 0.0;
  double outHalf = 0.0;
  double inCursor = 0.0;
  double outCursor = 0.0;
};
