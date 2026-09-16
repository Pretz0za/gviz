#include "render/renderer.hpp"

#include "render_internal.hpp"
#include "shaders.hpp"

#include "graph/components/edge.hpp"
#include "graph/types.hpp"
#include "layout/components/position.hpp"
#include "layout/types.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace {

struct GlobalsUBO {
  float viewProj[16];
  float viewport[2];
  float radiusPx;
  float edgeWidthPx;
  float nodeColor[4];
  float edgeColor[4];
};

void OrthoViewProj(float outMat[16], float cx, float cy, float halfW,
                    float halfH) {
  std::fill(outMat, outMat + 16, 0.0f);
  outMat[0] = 1.0f / halfW;
  outMat[5] = 1.0f / halfH;
  outMat[10] = 1.0f;
  outMat[12] = -cx / halfW;
  outMat[13] = -cy / halfH;
  outMat[15] = 1.0f;
}

void OnAdapterRequest(WGPURequestAdapterStatus, WGPUAdapter adapter,
                       WGPUStringView, void *userdata1, void *) {
  *static_cast<WGPUAdapter *>(userdata1) = adapter;
}

void OnDeviceRequest(WGPURequestDeviceStatus, WGPUDevice device,
                      WGPUStringView, void *userdata1, void *) {
  *static_cast<WGPUDevice *>(userdata1) = device;
}

void OnUncapturedError(WGPUDevice const *, WGPUErrorType, WGPUStringView message,
                       void *, void *) {
  std::fprintf(stderr, "[render] uncaptured WGPU error: %.*s\n",
              (int)message.length, message.data);
}

} // namespace

struct Renderer::Impl {
  GLFWwindow *window = nullptr;
  WGPUInstance instance = nullptr;
  WGPUSurface surface = nullptr;
  WGPUAdapter adapter = nullptr;
  WGPUDevice device = nullptr;
  WGPUQueue queue = nullptr;
  WGPUTextureFormat surfaceFormat = WGPUTextureFormat_Undefined;
  WGPUSurfaceConfiguration surfaceConfig{};

  WGPUShaderModule shaderModule = nullptr;
  WGPUBindGroupLayout bindGroupLayout = nullptr;
  WGPUPipelineLayout pipelineLayout = nullptr;
  WGPURenderPipeline nodePipeline = nullptr;
  WGPURenderPipeline edgePipeline = nullptr;

  WGPUBuffer globalsBuf = nullptr;
  WGPUBuffer positionsBuf = nullptr;
  WGPUBuffer nodeIdsBuf = nullptr;
  WGPUBuffer edgesBuf = nullptr;
  WGPUBindGroup bindGroup = nullptr;

  size_t positionsCapacity = 0;
  size_t nodeIdsCapacity = 0;
  size_t edgesCapacity = 0;
  bool bindGroupDirty = true;

  std::vector<float> positionsStaging;
  std::vector<uint32_t> nodeIdsStaging;
  std::vector<uint32_t> edgesStaging;

  float nodeRadiusPx = 32.0f;
  float edgeWidthPx = 2.0f;
  float nodeColor[4] = {0.85f, 0.85f, 0.95f, 1.0f};
  float edgeColor[4] = {0.45f, 0.5f, 0.6f, 0.8f};
  float bgColor[4] = {0.08f, 0.09f, 0.11f, 1.0f};

  ~Impl();

  bool Init(uint32_t width, uint32_t height, const std::string &title);
  bool CreatePipelines();
  WGPUBuffer CreateBuffer(size_t size, WGPUBufferUsage usage,
                          const char *label);
  bool EnsureBuffer(WGPUBuffer &buf, size_t &capacity, size_t neededBytes,
                    WGPUBufferUsage usage, const char *label);
  void RebuildBindGroup();
  void RecomputeFrameData(Graph &graph);
};

Renderer::Impl::~Impl() {
  if (bindGroup)
    wgpuBindGroupRelease(bindGroup);
  if (edgesBuf)
    wgpuBufferRelease(edgesBuf);
  if (nodeIdsBuf)
    wgpuBufferRelease(nodeIdsBuf);
  if (positionsBuf)
    wgpuBufferRelease(positionsBuf);
  if (globalsBuf)
    wgpuBufferRelease(globalsBuf);
  if (edgePipeline)
    wgpuRenderPipelineRelease(edgePipeline);
  if (nodePipeline)
    wgpuRenderPipelineRelease(nodePipeline);
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
  return wgpuDeviceCreateBuffer(
      device, WgpuPtr(WGPUBufferDescriptor{
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
      device, WgpuPtr(WGPUShaderModuleDescriptor{
                  .nextInChain = (WGPUChainedStruct *)WgpuPtr(
                      WGPUShaderSourceWGSL{
                          .chain = {.sType = WGPUSType_ShaderSourceWGSL},
                          .code = {kRenderWgslSource, WGPU_STRLEN},
                      }),
                  .label = {"render shaders", WGPU_STRLEN},
              }));
  if (!shaderModule)
    return false;

  WGPUBindGroupLayoutEntry entries[4] = {};
  entries[0] = WGPUBindGroupLayoutEntry{
      .binding = 0,
      .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
      .buffer = {.type = WGPUBufferBindingType_Uniform},
  };
  for (uint32_t i = 1; i < 4; i++) {
    entries[i] = WGPUBindGroupLayoutEntry{
        .binding = i,
        .visibility = WGPUShaderStage_Vertex,
        .buffer = {.type = WGPUBufferBindingType_ReadOnlyStorage},
    };
  }

  bindGroupLayout = wgpuDeviceCreateBindGroupLayout(
      device, WgpuPtr(WGPUBindGroupLayoutDescriptor{
                  .label = {"render bgl", WGPU_STRLEN},
                  .entryCount = 4,
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

void Renderer::Impl::RebuildBindGroup() {
  if (bindGroup)
    wgpuBindGroupRelease(bindGroup);

  WGPUBindGroupEntry entries[4] = {};
  entries[0] = WGPUBindGroupEntry{
      .binding = 0, .buffer = globalsBuf, .size = sizeof(GlobalsUBO)};
  entries[1] = WGPUBindGroupEntry{
      .binding = 1, .buffer = positionsBuf, .size = positionsCapacity};
  entries[2] = WGPUBindGroupEntry{
      .binding = 2, .buffer = nodeIdsBuf, .size = nodeIdsCapacity};
  entries[3] = WGPUBindGroupEntry{
      .binding = 3, .buffer = edgesBuf, .size = edgesCapacity};

  bindGroup = wgpuDeviceCreateBindGroup(
      device, WgpuPtr(WGPUBindGroupDescriptor{
                  .label = {"render bind group", WGPU_STRLEN},
                  .layout = bindGroupLayout,
                  .entryCount = 4,
                  .entries = entries,
              }));
  bindGroupDirty = false;
}

void Renderer::Impl::RecomputeFrameData(Graph &graph) {
  positionsStaging.clear();
  nodeIdsStaging.clear();
  edgesStaging.clear();

  auto *posPool = graph.NodeSpace().GetPool<PositionComponent>();
  if (!posPool)
    return;

  const VisibleNodesResource *visible =
      graph.HasResource<VisibleNodesResource>()
          ? graph.GetResource<VisibleNodesResource>()
          : nullptr;

  positionsStaging.reserve(graph.Size() * 2);
  for (NodeID nid : graph.Nodes()) {
    const PositionComponent *c = posPool->Find(nid.Raw());
    float x = c ? static_cast<float>(c->pos[0]) : 0.0f;
    float y = c ? static_cast<float>(c->pos[1]) : 0.0f;
    positionsStaging.push_back(x);
    positionsStaging.push_back(y);

    DenseNodeID did = graph.MapToDense(nid);
    bool isVisible =
        visible ? (did.Raw() < visible->Size() && visible->Test(did)) : true;
    if (isVisible)
      nodeIdsStaging.push_back(did.Raw());
  }

  for (EdgeID eid : graph.Edges()) {
    EdgeComponent e = graph.GetEdge(eid);
    DenseNodeID a = graph.MapToDense(e.from);
    DenseNodeID b = graph.MapToDense(e.to);
    bool aVisible =
        visible ? (a.Raw() < visible->Size() && visible->Test(a)) : true;
    bool bVisible =
        visible ? (b.Raw() < visible->Size() && visible->Test(b)) : true;
    if (aVisible && bVisible) {
      edgesStaging.push_back(a.Raw());
      edgesStaging.push_back(b.Raw());
    }
  }
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
      instance, WgpuPtr(WGPURequestAdapterOptions{.compatibleSurface = surface}),
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

  globalsBuf = CreateBuffer(sizeof(GlobalsUBO),
                            WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
                            "render globals");
  return globalsBuf != nullptr;
}

Renderer::Renderer(uint32_t width, uint32_t height, const std::string &title)
    : m_impl(std::make_unique<Impl>()) {
  if (!m_impl->Init(width, height, title)) {
    std::fprintf(stderr, "[render] Renderer initialization failed\n");
  }
}

Renderer::~Renderer() = default;

void Renderer::SetNodeRadiusPixels(float radius) {
  m_impl->nodeRadiusPx = radius;
}

void Renderer::SetEdgeWidthPixels(float width) { m_impl->edgeWidthPx = width; }

void Renderer::SetNodeColor(float r, float g, float b, float a) {
  m_impl->nodeColor[0] = r;
  m_impl->nodeColor[1] = g;
  m_impl->nodeColor[2] = b;
  m_impl->nodeColor[3] = a;
}

void Renderer::SetEdgeColor(float r, float g, float b, float a) {
  m_impl->edgeColor[0] = r;
  m_impl->edgeColor[1] = g;
  m_impl->edgeColor[2] = b;
  m_impl->edgeColor[3] = a;
}

void Renderer::SetBackgroundColor(float r, float g, float b, float a) {
  m_impl->bgColor[0] = r;
  m_impl->bgColor[1] = g;
  m_impl->bgColor[2] = b;
  m_impl->bgColor[3] = a;
}

bool Renderer::Frame(Graph &graph) {
  Impl &r = *m_impl;
  if (!r.window || glfwWindowShouldClose(r.window))
    return false;

  glfwPollEvents();

  int fbw, fbh;
  glfwGetFramebufferSize(r.window, &fbw, &fbh);
  if (fbw == 0 || fbh == 0)
    return true;
  if (static_cast<uint32_t>(fbw) != r.surfaceConfig.width ||
      static_cast<uint32_t>(fbh) != r.surfaceConfig.height) {
    r.surfaceConfig.width = static_cast<uint32_t>(fbw);
    r.surfaceConfig.height = static_cast<uint32_t>(fbh);
    wgpuSurfaceConfigure(r.surface, &r.surfaceConfig);
  }

  r.RecomputeFrameData(graph);

  size_t posBytes =
      std::max<size_t>(r.positionsStaging.size() * sizeof(float), 1);
  size_t nodeIdBytes =
      std::max<size_t>(r.nodeIdsStaging.size() * sizeof(uint32_t), 1);
  size_t edgeBytes =
      std::max<size_t>(r.edgesStaging.size() * sizeof(uint32_t), 1);

  r.EnsureBuffer(r.positionsBuf, r.positionsCapacity, posBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                "render positions");
  r.EnsureBuffer(r.nodeIdsBuf, r.nodeIdsCapacity, nodeIdBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                "render node ids");
  r.EnsureBuffer(r.edgesBuf, r.edgesCapacity, edgeBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                "render edges");

  if (!r.positionsStaging.empty())
    wgpuQueueWriteBuffer(r.queue, r.positionsBuf, 0, r.positionsStaging.data(),
                         r.positionsStaging.size() * sizeof(float));
  if (!r.nodeIdsStaging.empty())
    wgpuQueueWriteBuffer(r.queue, r.nodeIdsBuf, 0, r.nodeIdsStaging.data(),
                         r.nodeIdsStaging.size() * sizeof(uint32_t));
  if (!r.edgesStaging.empty())
    wgpuQueueWriteBuffer(r.queue, r.edgesBuf, 0, r.edgesStaging.data(),
                         r.edgesStaging.size() * sizeof(uint32_t));

  if (r.bindGroupDirty)
    r.RebuildBindGroup();

  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();
  for (uint32_t id : r.nodeIdsStaging) {
    float x = r.positionsStaging[id * 2];
    float y = r.positionsStaging[id * 2 + 1];
    minX = std::min(minX, x);
    minY = std::min(minY, y);
    maxX = std::max(maxX, x);
    maxY = std::max(maxY, y);
  }
  if (minX > maxX) {
    minX = -1.0f;
    maxX = 1.0f;
    minY = -1.0f;
    maxY = 1.0f;
  }

  float cx = (minX + maxX) * 0.5f;
  float cy = (minY + maxY) * 0.5f;
  float worldW = std::max(maxX - minX, 1e-3f) * 1.15f;
  float worldH = std::max(maxY - minY, 1e-3f) * 1.15f;
  float viewportAspect = static_cast<float>(fbw) / static_cast<float>(fbh);
  float halfW, halfH;
  if (worldW / worldH > viewportAspect) {
    halfW = worldW * 0.5f;
    halfH = halfW / viewportAspect;
  } else {
    halfH = worldH * 0.5f;
    halfW = halfH * viewportAspect;
  }

  GlobalsUBO globals{};
  OrthoViewProj(globals.viewProj, cx, cy, halfW, halfH);
  globals.viewport[0] = static_cast<float>(fbw);
  globals.viewport[1] = static_cast<float>(fbh);
  globals.radiusPx = r.nodeRadiusPx;
  globals.edgeWidthPx = r.edgeWidthPx;
  std::memcpy(globals.nodeColor, r.nodeColor, sizeof(globals.nodeColor));
  std::memcpy(globals.edgeColor, r.edgeColor, sizeof(globals.edgeColor));
  wgpuQueueWriteBuffer(r.queue, r.globalsBuf, 0, &globals, sizeof(globals));

  WGPUSurfaceTexture surfaceTexture;
  wgpuSurfaceGetCurrentTexture(r.surface, &surfaceTexture);
  if (surfaceTexture.status !=
          WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surfaceTexture.status !=
          WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
    if (surfaceTexture.texture)
      wgpuTextureRelease(surfaceTexture.texture);
    return true;
  }

  WGPUTextureView frame = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(
      r.device, WgpuPtr(WGPUCommandEncoderDescriptor{
                    .label = {"render", WGPU_STRLEN}}));

  WGPURenderPassColorAttachment colorAttachment{
      .view = frame,
      .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
      .loadOp = WGPULoadOp_Clear,
      .storeOp = WGPUStoreOp_Store,
      .clearValue = {r.bgColor[0], r.bgColor[1], r.bgColor[2], r.bgColor[3]},
  };
  WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(
      encoder, WgpuPtr(WGPURenderPassDescriptor{
                   .label = {"render pass", WGPU_STRLEN},
                   .colorAttachmentCount = 1,
                   .colorAttachments = &colorAttachment,
               }));

  wgpuRenderPassEncoderSetBindGroup(pass, 0, r.bindGroup, 0, nullptr);
  wgpuRenderPassEncoderSetPipeline(pass, r.edgePipeline);
  wgpuRenderPassEncoderDraw(
      pass, 6, static_cast<uint32_t>(r.edgesStaging.size() / 2), 0, 0);
  wgpuRenderPassEncoderSetPipeline(pass, r.nodePipeline);
  wgpuRenderPassEncoderDraw(
      pass, 6, static_cast<uint32_t>(r.nodeIdsStaging.size()), 0, 0);
  wgpuRenderPassEncoderEnd(pass);
  wgpuRenderPassEncoderRelease(pass);

  WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
  wgpuQueueSubmit(r.queue, 1, &commands);
  wgpuSurfacePresent(r.surface);

  wgpuCommandBufferRelease(commands);
  wgpuCommandEncoderRelease(encoder);
  wgpuTextureViewRelease(frame);
  wgpuTextureRelease(surfaceTexture.texture);
  return true;
}
