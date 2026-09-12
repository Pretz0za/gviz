#pragma once

#include <cstdint>

void RegularSimplexPoints(uint32_t dim, double sideLength, double *out);
// Copies the first `n` dim-wide rows of `in` into `out` (also dim-wide rows).
void FromDenseCoordinateArray(double *in, double *out, uint32_t dim,
                              uint32_t n);
