#pragma once

#include "ecs/handle.hpp"

struct NodeTag {};
struct EdgeTag {};
struct DenseNodeTag {};

using NodeID = Handle<NodeTag>;
using EdgeID = Handle<EdgeTag>;
using DenseNodeID = Handle<DenseNodeTag>;

inline constexpr NodeID INVALID_NODE_ID = NodeID(0xFFFFFFFF);
inline constexpr EdgeID INVALID_EDGE_ID = EdgeID(0xFFFFFFFF);
inline constexpr DenseNodeID INVALID_DENSE_NODE_ID = DenseNodeID(0xFFFFFFFF);
