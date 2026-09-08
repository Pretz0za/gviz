#pragma once

#include "ecs/components.hpp"

struct PositionComponent : Component {
	double pos[3] {0, 0, 0};
};
