#pragma once

#include <webgpu/webgpu.h>

typedef struct GLFWwindow GLFWwindow;

template <typename T> const T *WgpuPtr(const T &v) noexcept { return &v; }

WGPUSurface RenderPlatformCreateSurface(WGPUInstance instance,
                                        GLFWwindow *window);
