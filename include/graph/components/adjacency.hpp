#pragma once

#include <vector>

#include "ecs/components.hpp"
#include "graph/types.hpp"

struct AdjEntry {
  EdgeID edge;
  NodeID other;
};

struct OutAdjacencyComponent : Component {
  std::vector<AdjEntry> out;
};

struct InAdjacencyComponent : Component {
  std::vector<AdjEntry> in;
};
