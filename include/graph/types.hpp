#pragma once

#include "ecs/handle.hpp"

struct NodeTag {};
struct EdgeTag {};

using NodeID = Handle<NodeTag>;
using EdgeID = Handle<EdgeTag>;
