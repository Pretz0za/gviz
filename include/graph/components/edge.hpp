#pragma once

#include "ecs/components.hpp"
#include "graph/types.hpp"

struct EdgeComponent : Component {
  NodeID from;
  NodeID to;
};
