#pragma once

#include "ecs/handle.hpp"

struct NodeTag {};
struct EdgeTag {};

using NodeID = Handle<NodeTag>;
using EdgeID = Handle<EdgeTag>;

inline constexpr NodeID INVALID_NODE_ID = NodeID(0xFFFFFFFF);
inline constexpr EdgeID INVALID_EDGE_ID = EdgeID(0xFFFFFFFF);
