#pragma once

#include "ecs/handle.hpp"

// Tag types only used to distinguish Handle<> instantiations; never
// constructed.
struct NodeTag {};
struct EdgeTag {};

using NodeID = Handle<NodeTag>;
using EdgeID = Handle<EdgeTag>;
