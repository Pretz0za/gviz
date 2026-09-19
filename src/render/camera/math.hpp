#pragma once

#include <algorithm>
#include <cmath>

namespace rendermath {

inline void OrthoViewProj(float outMat[16], float cx, float cy, float halfW,
                          float halfH) {
  std::fill(outMat, outMat + 16, 0.0f);
  outMat[0] = 1.0f / halfW;
  outMat[5] = 1.0f / halfH;
  outMat[10] = 1.0f;
  outMat[12] = -cx / halfW;
  outMat[13] = -cy / halfH;
  outMat[15] = 1.0f;
}

// column-major, out = a * b
inline void Mat4Multiply(float out[16], const float a[16], const float b[16]) {
  float r[16];
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      float sum = 0.0f;
      for (int k = 0; k < 4; k++)
        sum += a[k * 4 + row] * b[col * 4 + k];
      r[col * 4 + row] = sum;
    }
  }
  std::copy(r, r + 16, out);
}

// right-handed perspective projection, WebGPU depth range [0, 1]
inline void PerspectiveProj(float outMat[16], float fovYRadians, float aspect,
                            float nearZ, float farZ) {
  std::fill(outMat, outMat + 16, 0.0f);
  float f = 1.0f / std::tan(fovYRadians * 0.5f);
  outMat[0] = f / aspect;
  outMat[5] = f;
  outMat[10] = farZ / (nearZ - farZ);
  outMat[11] = -1.0f;
  outMat[14] = (farZ * nearZ) / (nearZ - farZ);
}

inline void Vec3Sub(float out[3], const float a[3], const float b[3]) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

inline void Vec3Cross(float out[3], const float a[3], const float b[3]) {
  float r[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
               a[0] * b[1] - a[1] * b[0]};
  std::copy(r, r + 3, out);
}

inline float Vec3Dot(const float a[3], const float b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline void Vec3Normalize(float v[3]) {
  float len = std::sqrt(Vec3Dot(v, v));
  if (len < 1e-8f)
    return;
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

// right-handed look-at view matrix
inline void LookAtRH(float outMat[16], const float eye[3],
                     const float center[3], const float up[3]) {
  float f[3];
  Vec3Sub(f, center, eye);
  Vec3Normalize(f);
  float s[3];
  Vec3Cross(s, f, up);
  Vec3Normalize(s);
  float u[3];
  Vec3Cross(u, s, f);

  outMat[0] = s[0];
  outMat[4] = s[1];
  outMat[8] = s[2];
  outMat[12] = -Vec3Dot(s, eye);

  outMat[1] = u[0];
  outMat[5] = u[1];
  outMat[9] = u[2];
  outMat[13] = -Vec3Dot(u, eye);

  outMat[2] = -f[0];
  outMat[6] = -f[1];
  outMat[10] = -f[2];
  outMat[14] = Vec3Dot(f, eye);

  outMat[3] = 0.0f;
  outMat[7] = 0.0f;
  outMat[11] = 0.0f;
  outMat[15] = 1.0f;
}

} // namespace rendermath
