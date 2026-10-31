#pragma once

#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include <vector>

struct PlanarRotationAdjacencyComponent : Component {
  std::vector<AdjEntry> out;
};
