#pragma once

#include "ecs/components.hpp"
#include <cstdint>
struct DegreeComponent : Component {
  uint32_t in = 0;
  uint32_t out = 0;
};
