#pragma once

#include "ecs/components.hpp"

typedef struct VelocityComponent : Component {
  double vel[3] = {0.0, 0.0, 0.0};
} VelocityComponent;
