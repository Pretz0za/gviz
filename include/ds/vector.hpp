#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

constexpr double EPSILON = 1e-7;

inline bool IsZero(double scalar) { return std::fabs(scalar) < EPSILON; }

inline bool IsNan(const double *vec, uint8_t dim) {
  for (uint8_t i = 0; i < dim; i++) {
    if (!std::isnan(vec[i]))
      return false;
  }
  return true;
}

inline bool IsZero(const double *vec, uint8_t dim) {
  for (uint8_t i = 0; i < dim; i++) {
    if (std::fabs(vec[i]) > EPSILON)
      return false;
  }
  return true;
}

inline void Copy(const double *src, double *dst, uint8_t dim) {
  memcpy(dst, src, sizeof(double) * dim);
}

inline double DotProduct(const double *a, const double *b, uint8_t dim) {
  switch (dim) {
  case 2:
    return a[0] * b[0] + a[1] * b[1];
  case 3:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  case 4:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
  default: {
    double sum = 0.0;
    for (size_t i = 0; i < dim; i++)
      sum += a[i] * b[i];
    return sum;
  }
  }
}

inline double L2NormSquared(const double *vec, uint8_t dim) {
  return DotProduct(vec, vec, dim);
}

inline double L2Norm(const double *vec, uint8_t dim) {
  return sqrt(L2NormSquared(vec, dim));
}

inline void Scale(double *vec, double scalar, uint8_t dim) {
  switch (dim) {
  case 2:
    vec[0] *= scalar;
    vec[1] *= scalar;
    return;
  case 3:
    vec[0] *= scalar;
    vec[1] *= scalar;
    vec[2] *= scalar;
    return;
  case 4:
    vec[0] *= scalar;
    vec[1] *= scalar;
    vec[2] *= scalar;
    vec[3] *= scalar;
    return;
  default:
    for (size_t i = 0; i < dim; i++)
      vec[i] *= scalar;
  }
}
