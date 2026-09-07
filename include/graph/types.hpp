#pragma once

#include "ds/typed_bitset.hpp"
#include "ecs/handle.hpp"
#include <iterator>

struct NodeTag {};
struct EdgeTag {};
struct DenseNodeTag {};

using NodeID = Handle<NodeTag>;
using EdgeID = Handle<EdgeTag>;
using DenseNodeID = Handle<DenseNodeTag>;

inline constexpr NodeID INVALID_NODE_ID = NodeID(0xFFFFFFFF);
inline constexpr EdgeID INVALID_EDGE_ID = EdgeID(0xFFFFFFFF);
inline constexpr DenseNodeID INVALID_DENSE_NODE_ID = DenseNodeID(0xFFFFFFFF);

// Node-indexed bit sets, tagged by which index space they're keyed on.
using SparseNodeSet = TypedBitSet<NodeID>;   // keyed by parent-graph NodeID
using DenseNodeSet = TypedBitSet<DenseNodeID>; // keyed by compact DenseNodeID

static_assert(std::input_iterator<SparseNodeSet::iterator>);
static_assert(std::input_iterator<DenseNodeSet::iterator>);
