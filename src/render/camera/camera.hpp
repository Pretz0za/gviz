#pragma once

#include "camera/math.hpp"
#include <algorithm>
#include <cmath>

constexpr float kFovY = 0.6981317f; // 40 degrees
constexpr float kMinOrbitDistance = 1e-3f;
constexpr float kMinOrthoHalfHeight = 1e-3f;
constexpr float kMaxPitch = 1.5533f; // ~89 degrees

struct OrbitCamera {
  bool is3D = false;
  bool initialized = false;
  float target[3] = {0.0f, 0.0f, 0.0f};
  // 3D: orbit radius around target. 2D: half-height of the ortho view, in
  // world units.
  float distance = 1.0f;
  // 3D only; unused (and locked) in 2D mode.
  float yaw = -0.7f;
  float pitch = 0.5f;

  void Eye(float outEye[3]) const {
    float cp = std::cos(pitch);
    outEye[0] = target[0] + distance * cp * std::sin(yaw);
    outEye[1] = target[1] + distance * std::sin(pitch);
    outEye[2] = target[2] + distance * cp * std::cos(yaw);
  }

  void Orbit(float dYaw, float dPitch) {
    yaw += dYaw;
    pitch = std::clamp(pitch + dPitch, -kMaxPitch, kMaxPitch);
  }

  // Right/up basis nodes are billboarded against: the camera's actual right
  // and up vectors in 3D, or the fixed world axes in 2D (there's no roll to
  // account for).
  void Basis(float outRight[3], float outUp[3]) const {
    if (!is3D) {
      outRight[0] = 1.0f;
      outRight[1] = 0.0f;
      outRight[2] = 0.0f;
      outUp[0] = 0.0f;
      outUp[1] = 1.0f;
      outUp[2] = 0.0f;
      return;
    }
    float eye[3];
    Eye(eye);
    float fwd[3];
    rendermath::Vec3Sub(fwd, target, eye);
    rendermath::Vec3Normalize(fwd);
    float worldUp[3] = {0.0f, 1.0f, 0.0f};
    rendermath::Vec3Cross(outRight, fwd, worldUp);
    rendermath::Vec3Normalize(outRight);
    rendermath::Vec3Cross(outUp, outRight, fwd);
  }

  // World-space size of one screen pixel, at the target's distance from the
  // camera. Used to size the pan drag and node antialiasing consistently
  // with the current zoom level.
  float WorldPerPixel(float viewportHeight) const {
    return is3D ? 2.0f * distance * std::tan(kFovY * 0.5f) / viewportHeight
               : 2.0f * distance / viewportHeight;
  }

  // Pans the target across the camera's right/up plane, so drag speed
  // matches what's on screen.
  void Pan(float dxPixels, float dyPixels, float viewportHeight) {
    float right[3], up[3];
    Basis(right, up);
    float worldPerPixel = WorldPerPixel(viewportHeight);
    for (int i = 0; i < 3; i++)
      target[i] +=
          (-right[i] * dxPixels + up[i] * dyPixels) * worldPerPixel;
  }

  void Zoom(float scrollY) {
    float factor = std::pow(0.9f, scrollY);
    distance *= factor;
    float minDist = is3D ? kMinOrbitDistance : kMinOrthoHalfHeight;
    distance = std::max(distance, minDist);
  }
};
