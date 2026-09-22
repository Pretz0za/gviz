#pragma once

#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include <concepts>
template <typename F>
concept ForceModel = requires(F forceModel, DenseNodeID denseId, const double* pos, double dbl) {
  { forceModel.RepulsiveTick(denseId, denseId) } -> std::same_as<void>;
  { forceModel.RepulsiveTick(denseId, pos, dbl) } -> std::same_as<void>;
  { forceModel.AttractiveTick(denseId, denseId) } -> std::same_as<void>;
};
