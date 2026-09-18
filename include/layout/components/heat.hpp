#pragma once

#include "ecs/components.hpp"

typedef struct LocalHeatComponent : Component {
  double heat{0};
  double oldCos{0};
} LocalHeatComponent;
