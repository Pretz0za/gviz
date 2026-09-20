#pragma once

#include "ecs/components.hpp"

typedef struct ForceAtlasHeatComponent : Component {
  double oldForce[3]{0.0, 0.0, 0.0};
  double swinging{0.0};
} ForceAtlasHeatComponent;
