#include "camera/camera.hpp"
#include "camera/math.hpp"
#include "renderer_impl.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float kOrbitSensitivity = 0.006f; // radians per pixel
} // namespace

void Renderer::Impl::FrameCameraIfNeeded(const FrameData &fd) {
  if (camera.initialized || !fd.bboxValid)
    return;

  float center[3];
  for (int i = 0; i < 3; i++)
    center[i] = (fd.bboxMin[i] + fd.bboxMax[i]) * 0.5f;
  std::copy(center, center + 3, camera.target);

  if (camera.is3D) {
    float ext[3];
    for (int i = 0; i < 3; i++)
      ext[i] = fd.bboxMax[i] - fd.bboxMin[i];
    float radius =
        0.5f * std::sqrt(ext[0] * ext[0] + ext[1] * ext[1] + ext[2] * ext[2]);
    radius = std::max(radius, 1e-3f);
    camera.distance = radius / std::sin(kFovY * 0.5f) * 1.15f;
  } else {
    float halfW = std::max((fd.bboxMax[0] - fd.bboxMin[0]) * 0.5f, 1e-3f) * 1.15f;
    float halfH = std::max((fd.bboxMax[1] - fd.bboxMin[1]) * 0.5f, 1e-3f) * 1.15f;
    camera.distance = std::max(halfW, halfH);
  }
  camera.initialized = true;
}

void Renderer::Impl::ComputeViewProj(float outMat[16], uint32_t fbw,
                                     uint32_t fbh) const {
  float aspect = static_cast<float>(fbw) / static_cast<float>(fbh);

  if (!camera.is3D) {
    float halfH = std::max(camera.distance, kMinOrthoHalfHeight);
    float halfW = halfH * aspect;
    rendermath::OrthoViewProj(outMat, camera.target[0], camera.target[1],
                              halfW, halfH);
    return;
  }

  float eye[3];
  camera.Eye(eye);
  const float up[3] = {0.0f, 1.0f, 0.0f};
  float view[16];
  rendermath::LookAtRH(view, eye, camera.target, up);

  float nearZ = std::max(0.01f, camera.distance * 0.01f);
  float farZ = std::max(nearZ * 10.0f, camera.distance * 100.0f);
  float proj[16];
  rendermath::PerspectiveProj(proj, kFovY, aspect, nearZ, farZ);

  rendermath::Mat4Multiply(outMat, proj, view);
}

void Renderer::Impl::OnMouseButton(GLFWwindow *window, int button, int action,
                                   int) {
  auto *impl = static_cast<Impl *>(glfwGetWindowUserPointer(window));
  if (!impl)
    return;
  bool pressed = action == GLFW_PRESS;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    impl->orbitDragging = pressed;
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT ||
            button == GLFW_MOUSE_BUTTON_MIDDLE) {
    impl->panDragging = pressed;
  }
  if (!pressed && !impl->orbitDragging && !impl->panDragging)
    impl->haveLastCursor = false;
}

void Renderer::Impl::OnCursorPos(GLFWwindow *window, double x, double y) {
  auto *impl = static_cast<Impl *>(glfwGetWindowUserPointer(window));
  if (!impl)
    return;
  if (!impl->haveLastCursor) {
    impl->lastCursorX = x;
    impl->lastCursorY = y;
    impl->haveLastCursor = true;
    return;
  }
  float dx = static_cast<float>(x - impl->lastCursorX);
  float dy = static_cast<float>(y - impl->lastCursorY);
  impl->lastCursorX = x;
  impl->lastCursorY = y;

  int fbh = 0, fbw = 0;
  glfwGetFramebufferSize(window, &fbw, &fbh);
  if (fbh <= 0)
    return;

  if (impl->orbitDragging && impl->camera.is3D) {
    impl->camera.Orbit(dx * kOrbitSensitivity, -dy * kOrbitSensitivity);
  } else if (impl->panDragging ||
            (impl->orbitDragging && !impl->camera.is3D)) {
    impl->camera.Pan(dx, dy, static_cast<float>(fbh));
  }
}

void Renderer::Impl::OnScroll(GLFWwindow *window, double, double yoffset) {
  auto *impl = static_cast<Impl *>(glfwGetWindowUserPointer(window));
  if (!impl)
    return;
  impl->camera.Zoom(static_cast<float>(yoffset));
}
