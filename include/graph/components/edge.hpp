#pragma once

#include "ecs/components.hpp"
#include "graph/types.hpp"

struct EdgeComponent : Component {
  NodeID from;
  NodeID to;

  bool operator==(const EdgeComponent &other) const {
    return from == other.from && to == other.to;
  }
};

inline constexpr EdgeComponent INVALID_EDGE =
    EdgeComponent{{},
                  NodeID(std::numeric_limits<uint32_t>::max()),
                  NodeID(std::numeric_limits<uint32_t>::max())};
