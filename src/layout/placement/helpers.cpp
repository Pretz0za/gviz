#include "layout/placement/helpers.hpp"
#include <cassert>

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

void FromDenseCoordinateArray(double *in, PositionComponent *out, uint32_t dim,
                              uint32_t n) {
  assert(dim == 2 || dim == 3);
  int d3 = dim == 3;
  for (uint32_t i = 0; i < n; i++) {
    out[i] = {{},
              {in[dim * i + 0], in[dim * i + 1], d3 ? in[dim * i + 2] : 0.0}};
  }
}
