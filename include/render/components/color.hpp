#pragma once

#include "ecs/components.hpp"
#include <cstdint>

#define DEFAULT_COLOR 0xFFFFFFFFu

struct ColorComponent : Component {
  uint32_t color = DEFAULT_COLOR;
};
