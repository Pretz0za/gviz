#pragma once

#include "ecs/dimensional_component.hpp"

struct PositionTag {};

using PositionComponent = DimensionalComponent<PositionTag>;
using PositionSpan = DimensionalComponentSpan<PositionTag>;
