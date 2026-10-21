#pragma once

#include "ecs/components.hpp"

typedef struct SpringComponent : Component {
  double stiffness = 1.0;
  double restLength = 1.0;
} SpringComponent;
