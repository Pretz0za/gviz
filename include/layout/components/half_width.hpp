#pragma once

#include "ecs/components.hpp"

#define DEFAULT_HALF_WIDTH 1.0

struct HalfWidthComponent : Component {
  double halfWidth = 0.0;
};
