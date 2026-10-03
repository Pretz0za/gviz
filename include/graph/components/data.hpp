#pragma once

#include "ecs/components.hpp"

template <typename T> struct DataComponent : Component {
  T data{};
};
