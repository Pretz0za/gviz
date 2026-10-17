#pragma once

#include <cstdint>
#include <webgpu/webgpu.h>

typedef struct GLFWwindow GLFWwindow;

inline constexpr uint32_t kBezierSegmentsPerCurve = 24;

template <typename T> const T *WgpuPtr(const T &v) noexcept { return &v; }

WGPUSurface RenderPlatformCreateSurface(WGPUInstance instance,
                                        GLFWwindow *window);
