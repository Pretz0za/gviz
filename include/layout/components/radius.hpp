#pragma once

#include "ecs/components.hpp"

#define DEFAULT_RADIUS 8.0

typedef struct RadiusComponent : Component {
	double radius = DEFAULT_RADIUS;
} RadiusComponent;
