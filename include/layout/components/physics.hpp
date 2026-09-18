#pragma once

#include "ecs/components.hpp"
#include <limits>
typedef struct PhysicsComponent : Component {
  double disp[3];
  double oldDisp[3]{std::numeric_limits<double>::quiet_NaN(),
                    std::numeric_limits<double>::quiet_NaN(),
                    std::numeric_limits<double>::quiet_NaN()};
} PhysicsComponent;
