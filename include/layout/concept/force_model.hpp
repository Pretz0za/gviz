#pragma once

#include "graph/types.hpp"
#include <concepts>
template <typename F>
concept ForceModel = requires(F forceModel, DenseNodeID denseId) {
  { forceModel.RepulsiveTick(denseId, denseId) } -> std::same_as<void>;
  { forceModel.AttractiveTick(denseId, denseId) } -> std::same_as<void>;
};
