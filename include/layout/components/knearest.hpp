#pragma once

#include "graph/search/types.hpp"
#include <cstdint>

template <uint32_t K> struct KNearestComponent {
  FoundNode nearest[K];
  uint32_t size;
};
