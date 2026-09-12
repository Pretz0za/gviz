#include "layout/placement/helpers.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

void RegularSimplexPoints(uint32_t dim, double sideLength, double *out) {
  std::fill(out, out + (dim + 1) * dim, 0.0);

  for (size_t k = 0; k <= dim; k++) {
    double *vk = out + k * dim;

    if (k == 0) {
      vk[0] = 1.0;
      continue;
    }

    double c = -1.0 / static_cast<double>(dim);

    for (size_t j = 0; j < k; j++) {
      double *vj = out + j * dim;
      double dotSoFar = 0.0;
      for (size_t m = 0; m < j; m++)
        dotSoFar += vk[m] * vj[m];
      vk[j] = (c - dotSoFar) / vj[j];
    }

    double normSq = 0.0;
    for (size_t m = 0; m < k; m++)
      normSq += vk[m] * vk[m];
    if (k < dim) {
      double rem = 1.0 - normSq;
      vk[k] = rem > 0.0 ? std::sqrt(rem) : 0.0;
    }
  }

  double *v0 = out;
  double *v1 = out + dim;
  double unitEdge = 0.0;
  for (size_t m = 0; m < dim; m++) {
    double d = v0[m] - v1[m];
    unitEdge += d * d;
  }
  unitEdge = std::sqrt(unitEdge);

  double scale = sideLength / unitEdge;
  for (size_t i = 0; i < (dim + 1) * dim; i++)
    out[i] *= scale;
}

void FromDenseCoordinateArray(double *in, double *out, uint32_t dim,
                              uint32_t n) {
  assert(dim == 2 || dim == 3);
  std::memcpy(out, in, sizeof(double) * dim * n);
}
