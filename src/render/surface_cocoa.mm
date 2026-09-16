#include "render_internal.hpp"

#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

WGPUSurface RenderPlatformCreateSurface(WGPUInstance instance,
                                        GLFWwindow *window) {
  NSWindow *nsWindow = glfwGetCocoaWindow(window);
  [nsWindow.contentView setWantsLayer:YES];
  CAMetalLayer *metalLayer = [CAMetalLayer layer];
  [nsWindow.contentView setLayer:metalLayer];

  return wgpuInstanceCreateSurface(
      instance,
      WgpuPtr(WGPUSurfaceDescriptor{
          .nextInChain =
              (WGPUChainedStruct *)WgpuPtr(WGPUSurfaceSourceMetalLayer{
                  .chain = {.sType = WGPUSType_SurfaceSourceMetalLayer},
                  .layer = metalLayer,
              }),
      }));
}
