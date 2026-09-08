#pragma once

#include "layout/components/position.hpp"

void RegularSimplexPoints(uint32_t dim, double sideLength, double *out);
void FromDenseCoordinateArray(double *in, PositionComponent *out, uint32_t dim,
                              uint32_t n);
