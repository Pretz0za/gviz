#pragma once

#include "ecs/components.hpp"

struct CubicBezierComponent : Component {
  double p0[2]{};
  double p1[2]{};
  double p2[2]{};
  double p3[2]{};
};
