#include "gpu/shaders.hpp"
#include "renderer_impl.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>

namespace {

void OnAdapterRequest(WGPURequestAdapterStatus, WGPUAdapter adapter,
                      WGPUStringView, void *userdata1, void *) {
  *static_cast<WGPUAdapter *>(userdata1) = adapter;
}

void OnDeviceRequest(WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView,
                     void *userdata1, void *) {
  *static_cast<WGPUDevice *>(userdata1) = device;
}

void OnUncapturedError(WGPUDevice const *, WGPUErrorType,
                       WGPUStringView message, void *, void *) {
  std::fprintf(stderr, "[render] uncaptured WGPU error: %.*s\n",
               (int)message.length, message.data);
}

} // namespace

Renderer::Impl::~Impl() {
  if (bindGroup)
    wgpuBindGroupRelease(bindGroup);
  if (edgesBuf)
    wgpuBufferRelease(edgesBuf);
  if (nodeIdsBuf)
    wgpuBufferRelease(nodeIdsBuf);
  if (positionsBuf)
    wgpuBufferRelease(positionsBuf);
  if (radiusBuf)
    wgpuBufferRelease(radiusBuf);
  if (globalsBuf)
    wgpuBufferRelease(globalsBuf);
  if (edgePipeline)
    wgpuRenderPipelineRelease(edgePipeline);
  if (nodePipeline)
    wgpuRenderPipelineRelease(nodePipeline);
  if (depthView)
    wgpuTextureViewRelease(depthView);
  if (depthTexture)
    wgpuTextureRelease(depthTexture);
  if (pipelineLayout)
    wgpuPipelineLayoutRelease(pipelineLayout);
  if (bindGroupLayout)
    wgpuBindGroupLayoutRelease(bindGroupLayout);
  if (shaderModule)
    wgpuShaderModuleRelease(shaderModule);
  if (queue)
    wgpuQueueRelease(queue);
  if (device)
    wgpuDeviceRelease(device);
  if (adapter)
    wgpuAdapterRelease(adapter);
  if (surface)
    wgpuSurfaceRelease(surface);
  if (instance)
    wgpuInstanceRelease(instance);
  if (window)
    glfwDestroyWindow(window);
  glfwTerminate();
}

WGPUBuffer Renderer::Impl::CreateBuffer(size_t size, WGPUBufferUsage usage,
                                        const char *label) {
  return wgpuDeviceCreateBuffer(device, WgpuPtr(WGPUBufferDescriptor{
                                            .label = {label, WGPU_STRLEN},
                                            .usage = usage,
                                            .size = size,
                                        }));
}

bool Renderer::Impl::EnsureBuffer(WGPUBuffer &buf, size_t &capacity,
                                  size_t neededBytes, WGPUBufferUsage usage,
                                  const char *label) {
  if (buf && neededBytes <= capacity)
    return true;
  if (buf)
    wgpuBufferRelease(buf);
  capacity = std::max<size_t>(neededBytes, 64);
  buf = CreateBuffer(capacity, usage, label);
  bindGroupDirty = true;
  return buf != nullptr;
}

bool Renderer::Impl::CreatePipelines() {
  shaderModule = wgpuDeviceCreateShaderModule(
      device,
      WgpuPtr(WGPUShaderModuleDescriptor{
          .nextInChain = (WGPUChainedStruct *)WgpuPtr(WGPUShaderSourceWGSL{
              .chain = {.sType = WGPUSType_ShaderSourceWGSL},
              .code = {kRenderWgslSource, WGPU_STRLEN},
          }),
          .label = {"render shaders", WGPU_STRLEN},
      }));
  if (!shaderModule)
    return false;

  WGPUBindGroupLayoutEntry entries[5] = {};
  entries[0] = WGPUBindGroupLayoutEntry{
      .binding = 0,
      .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
      .buffer = {.type = WGPUBufferBindingType_Uniform},
  };
  for (uint32_t i = 1; i < 5; i++) {
    entries[i] = WGPUBindGroupLayoutEntry{
        .binding = i,
        .visibility = WGPUShaderStage_Vertex,
        .buffer = {.type = WGPUBufferBindingType_ReadOnlyStorage},
    };
  }

  bindGroupLayout = wgpuDeviceCreateBindGroupLayout(
      device, WgpuPtr(WGPUBindGroupLayoutDescriptor{
                  .label = {"render bgl", WGPU_STRLEN},
                  .entryCount = 5,
                  .entries = entries,
              }));
  pipelineLayout = wgpuDeviceCreatePipelineLayout(
      device, WgpuPtr(WGPUPipelineLayoutDescriptor{
                  .label = {"render layout", WGPU_STRLEN},
                  .bindGroupLayoutCount = 1,
                  .bindGroupLayouts = &bindGroupLayout,
              }));
  if (!bindGroupLayout || !pipelineLayout)
    return false;

  const WGPUBlendState blend = {
      .color = {.operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha},
      .alpha = {.operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_One,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha},
  };
  const WGPUColorTargetState colorTarget = {
      .format = surfaceFormat,
      .blend = &blend,
      .writeMask = WGPUColorWriteMask_All,
  };

  // LessEqual (not Less): nodes are drawn after edges and commonly share the
  // exact same depth as an edge endpoint (e.g. every node in 2D, where z is
  // always 0). With strict Less, the later draw at an equal depth loses the
  // test and never appears, so edges would occlude the nodes sitting right
  // on top of them.
  const WGPUDepthStencilState depthStencil = {
      .format = depthFormat,
      .depthWriteEnabled = WGPUOptionalBool_True,
      .depthCompare = WGPUCompareFunction_LessEqual,
  };

  const char *labels[2] = {"render nodes", "render edges"};
  const char *vsEntries[2] = {"vsNode", "vsEdge"};
  const char *fsEntries[2] = {"fsNode", "fsEdge"};
  WGPURenderPipeline pipelines[2] = {nullptr, nullptr};

  for (int i = 0; i < 2; i++) {
    pipelines[i] = wgpuDeviceCreateRenderPipeline(
        device,
        WgpuPtr(WGPURenderPipelineDescriptor{
            .label = {labels[i], WGPU_STRLEN},
            .layout = pipelineLayout,
            .vertex = {.module = shaderModule,
                       .entryPoint = {vsEntries[i], WGPU_STRLEN}},
            .primitive = {.topology = WGPUPrimitiveTopology_TriangleList,
                          .cullMode = WGPUCullMode_None},
            .depthStencil = &depthStencil,
            .multisample = {.count = 1, .mask = 0xFFFFFFFF},
            .fragment = WgpuPtr(WGPUFragmentState{
                .module = shaderModule,
                .entryPoint = {fsEntries[i], WGPU_STRLEN},
                .targetCount = 1,
                .targets = &colorTarget,
            }),
        }));
    if (!pipelines[i])
      return false;
  }
  nodePipeline = pipelines[0];
  edgePipeline = pipelines[1];
  return true;
}

bool Renderer::Impl::CreateDepthTexture(uint32_t width, uint32_t height) {
  if (depthView)
    wgpuTextureViewRelease(depthView);
  if (depthTexture)
    wgpuTextureRelease(depthTexture);

  depthTexture = wgpuDeviceCreateTexture(
      device, WgpuPtr(WGPUTextureDescriptor{
                  .label = {"render depth", WGPU_STRLEN},
                  .usage = WGPUTextureUsage_RenderAttachment,
                  .dimension = WGPUTextureDimension_2D,
                  .size = {width, height, 1},
                  .format = depthFormat,
                  .mipLevelCount = 1,
                  .sampleCount = 1,
              }));
  if (!depthTexture)
    return false;

  depthView = wgpuTextureCreateView(depthTexture, nullptr);
  return depthView != nullptr;
}

bool Renderer::Impl::Init(uint32_t width, uint32_t height,
                          const std::string &title) {
  if (!glfwInit())
    return false;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                            title.c_str(), nullptr, nullptr);
  if (!window)
    return false;

  instance = wgpuCreateInstance(nullptr);
  if (!instance)
    return false;

  surface = RenderPlatformCreateSurface(instance, window);
  if (!surface)
    return false;

  wgpuInstanceRequestAdapter(
      instance,
      WgpuPtr(WGPURequestAdapterOptions{.compatibleSurface = surface}),
      WGPURequestAdapterCallbackInfo{.callback = OnAdapterRequest,
                                     .userdata1 = &adapter});
  if (!adapter)
    return false;

  WGPULimits adapterLimits = WGPU_LIMITS_INIT;
  wgpuAdapterGetLimits(adapter, &adapterLimits);

  WGPULimits requiredLimits = WGPU_LIMITS_INIT;
  requiredLimits.maxStorageBufferBindingSize =
      adapterLimits.maxStorageBufferBindingSize;
  requiredLimits.maxBufferSize = adapterLimits.maxBufferSize;

  wgpuAdapterRequestDevice(
      adapter,
      WgpuPtr(WGPUDeviceDescriptor{
          .label = {"render device", WGPU_STRLEN},
          .requiredLimits = &requiredLimits,
          .uncapturedErrorCallbackInfo = {.callback = OnUncapturedError},
      }),
      WGPURequestDeviceCallbackInfo{.callback = OnDeviceRequest,
                                    .userdata1 = &device});
  if (!device)
    return false;

  queue = wgpuDeviceGetQueue(device);

  WGPUSurfaceCapabilities caps = {};
  wgpuSurfaceGetCapabilities(surface, adapter, &caps);
  surfaceFormat = caps.formats[0];
  for (size_t i = 0; i < caps.formatCount; i++) {
    if (caps.formats[i] == WGPUTextureFormat_BGRA8Unorm ||
        caps.formats[i] == WGPUTextureFormat_RGBA8Unorm) {
      surfaceFormat = caps.formats[i];
      break;
    }
  }

  int fbw, fbh;
  glfwGetFramebufferSize(window, &fbw, &fbh);
  surfaceConfig = WGPUSurfaceConfiguration{
      .device = device,
      .format = surfaceFormat,
      .usage = WGPUTextureUsage_RenderAttachment,
      .width = static_cast<uint32_t>(fbw),
      .height = static_cast<uint32_t>(fbh),
      .alphaMode = caps.alphaModes[0],
      .presentMode = WGPUPresentMode_Fifo,
  };
  wgpuSurfaceCapabilitiesFreeMembers(caps);
  wgpuSurfaceConfigure(surface, &surfaceConfig);

  if (!CreatePipelines())
    return false;
  if (!CreateDepthTexture(surfaceConfig.width, surfaceConfig.height))
    return false;

  globalsBuf = CreateBuffer(sizeof(GlobalsUBO),
                            WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
                            "render globals");
  if (!globalsBuf)
    return false;

  glfwSetWindowUserPointer(window, this);
  glfwSetMouseButtonCallback(window, OnMouseButton);
  glfwSetCursorPosCallback(window, OnCursorPos);
  glfwSetScrollCallback(window, OnScroll);
  return true;
}
