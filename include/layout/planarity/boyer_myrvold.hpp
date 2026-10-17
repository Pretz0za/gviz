#pragma once

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

class NotPlanarException : public std::runtime_error {
public:
  NotPlanarException() : std::runtime_error("graph is not planar") {}
};

std::vector<std::vector<uint32_t>>
ComputePlanarRotation(uint32_t vertexCount,
                       const std::vector<std::pair<uint32_t, uint32_t>> &edges);
