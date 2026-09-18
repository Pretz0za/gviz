#pragma once

#include "ecs/components.hpp"

#define DEFAULT_RADIUS 64.0

typedef struct RadiusComponent : Component {
	double radius = DEFAULT_RADIUS;
} RadiusComponent;
