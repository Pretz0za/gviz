#include "ecs/components.hpp"
#include <vector>

struct AdjacencyComponent : Component {
  std::vector<EntityID> neighbors;
};
