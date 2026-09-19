#include "render_internal.hpp"

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

WGPUSurface RenderPlatformCreateSurface(WGPUInstance instance,
                                        GLFWwindow *window) {
  if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
    return wgpuInstanceCreateSurface(
        instance,
        WgpuPtr(WGPUSurfaceDescriptor{
            .nextInChain =
                (WGPUChainedStruct *)WgpuPtr(WGPUSurfaceSourceWaylandSurface{
                    .chain = {.sType = WGPUSType_SurfaceSourceWaylandSurface},
                    .display = glfwGetWaylandDisplay(),
                    .surface = glfwGetWaylandWindow(window),
                }),
        }));
  }

  return wgpuInstanceCreateSurface(
      instance,
      WgpuPtr(WGPUSurfaceDescriptor{
          .nextInChain =
              (WGPUChainedStruct *)WgpuPtr(WGPUSurfaceSourceXlibWindow{
                  .chain = {.sType = WGPUSType_SurfaceSourceXlibWindow},
                  .display = glfwGetX11Display(),
                  .window = glfwGetX11Window(window),
              }),
      }));
}
