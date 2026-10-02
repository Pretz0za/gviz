#pragma once

#include "ecs/components.hpp"
#include <cstdint>

struct RadialEdgeDecorator : Component {
  double slope = 0.0;
  double length = 0.0;
  uint32_t order = 0;
};
