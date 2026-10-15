#pragma once

#include "ecs/dimensional_component.hpp"

// Anchors used by PositionAnimationSystem to interpolate PositionComponent
// over time: AnimationFromPositionComponent is where the transition started,
// AnimationToPositionComponent is where it's headed.

struct AnimationFromPositionTag {};
struct AnimationToPositionTag {};

using AnimationFromPositionComponent =
    DimensionalComponent<AnimationFromPositionTag>;
using AnimationFromPositionSpan = DimensionalComponentSpan<AnimationFromPositionTag>;

using AnimationToPositionComponent =
    DimensionalComponent<AnimationToPositionTag>;
using AnimationToPositionSpan = DimensionalComponentSpan<AnimationToPositionTag>;
