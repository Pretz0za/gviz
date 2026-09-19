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
  switch (dim) {
  case 2:
    return IsZero(vec[0]) && IsZero(vec[1]);
  case 3:
    return IsZero(vec[0]) && IsZero(vec[1]) && IsZero(vec[2]);
  case 4:
    return IsZero(vec[0]) && IsZero(vec[1]) && IsZero(vec[2]) && IsZero(vec[3]);
  default:
    for (uint8_t i = 0; i < dim; i++) {
      if (std::fabs(vec[i]) > EPSILON)
        return false;
    }
    return true;
  }
}

inline void ZeroOut(double *vec, uint8_t dim) {
  for (uint8_t i = 0; i < dim; i++) {
    vec[i] = 0.0;
  }
}

// y = ax + y
inline void Vecaxpy(double alpha, const double *x, double *y,
                    uint8_t dimension) {
  switch (dimension) {
  case 2:
    y[0] += alpha * x[0];
    y[1] += alpha * x[1];
    return;
  case 3:
    y[0] += alpha * x[0];
    y[1] += alpha * x[1];
    y[2] += alpha * x[2];
    return;
  case 4:
    y[0] += alpha * x[0];
    y[1] += alpha * x[1];
    y[2] += alpha * x[2];
    y[3] += alpha * x[3];
    return;
  default: {
    for (uint8_t i = 0; i < dimension; i++) {
      y[i] += alpha * x[i];
    }
    return;
  }
  }
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
    for (uint8_t i = 0; i < dim; i++)
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

// a - b -> out
inline void Subtract(const double *a, const double *b, double *out,
                     uint8_t dim) {
  switch (dim) {
  case 2: {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    return;
  }
  case 3: {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
    return;
  }
  default: {
    for (uint8_t i = 0; i < dim; i++) {
      out[i] = a[i] - b[i];
    }
  }
  }
}

inline double Distance(const double *a, const double *b, uint8_t dim) {
  switch (dim) {
  case 2: {
    double dx = a[0] - b[0];
    double dy = a[1] - b[1];
    return sqrt(dx * dx + dy * dy);
  }
  case 3: {
    double dx = a[0] - b[0];
    double dy = a[1] - b[1];
    double dz = a[2] - b[2];
    return sqrt(dx * dx + dy * dy + dz * dz);
  }
  default: {
    double distSq = 0.0;
    for (uint8_t i = 0; i < dim; i++) {
      double d = a[i] - b[i];
      distSq += d * d;
    }
    return sqrt(distSq);
  }
  }
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
    for (uint8_t i = 0; i < dim; i++)
      vec[i] *= scalar;
  }
}
