#include "render/renderer.hpp"

#include "ecs/exceptions.hpp"
#include "layout/components/radius.hpp"
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
#include <webgpu/webgpu.h>

namespace {

struct GlobalsUBO {
  float viewProj[16];
  float viewport[2];
  float edgeWidthPx;
  float worldPerPixel; // world-space size of one screen pixel; used to keep
                       // node antialiasing ~1px wide regardless of zoom
  float nodeColor[4];
  float edgeColor[4];
  float cameraRight[3]; // billboard basis nodes are expanded along, in world
                        // space, so node radius is a world-space size
  float _padR;          // WGSL aligns the next vec3f to 16 bytes
  float cameraUp[3];
  float _padU; // WGSL rounds the struct size up to a multiple of 16 bytes
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

// column-major, out = a * b
void Mat4Multiply(float out[16], const float a[16], const float b[16]) {
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
void PerspectiveProj(float outMat[16], float fovYRadians, float aspect,
                     float nearZ, float farZ) {
  std::fill(outMat, outMat + 16, 0.0f);
  float f = 1.0f / std::tan(fovYRadians * 0.5f);
  outMat[0] = f / aspect;
  outMat[5] = f;
  outMat[10] = farZ / (nearZ - farZ);
  outMat[11] = -1.0f;
  outMat[14] = (farZ * nearZ) / (nearZ - farZ);
}

void Vec3Sub(float out[3], const float a[3], const float b[3]) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

void Vec3Cross(float out[3], const float a[3], const float b[3]) {
  float r[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
               a[0] * b[1] - a[1] * b[0]};
  std::copy(r, r + 3, out);
}

float Vec3Dot(const float a[3], const float b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Vec3Normalize(float v[3]) {
  float len = std::sqrt(Vec3Dot(v, v));
  if (len < 1e-8f)
    return;
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

// right-handed look-at view matrix
void LookAtRH(float outMat[16], const float eye[3], const float center[3],
             const float up[3]) {
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
    Vec3Sub(fwd, target, eye);
    Vec3Normalize(fwd);
    float worldUp[3] = {0.0f, 1.0f, 0.0f};
    Vec3Cross(outRight, fwd, worldUp);
    Vec3Normalize(outRight);
    Vec3Cross(outUp, outRight, fwd);
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

  WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus;
  WGPUTexture depthTexture = nullptr;
  WGPUTextureView depthView = nullptr;

  WGPUBuffer globalsBuf = nullptr;
  WGPUBuffer positionsBuf = nullptr;
  WGPUBuffer nodeIdsBuf = nullptr;
  WGPUBuffer edgesBuf = nullptr;
  WGPUBuffer radiusBuf = nullptr;
  WGPUBindGroup bindGroup = nullptr;

  size_t positionsCapacity = 0;
  size_t nodeIdsCapacity = 0;
  size_t edgesCapacity = 0;
  size_t radiusCapacity = 0;
  bool bindGroupDirty = true;

  std::vector<float> positionsStaging;
  std::vector<uint32_t> nodeIdsStaging;
  std::vector<uint32_t> edgesStaging;
  std::vector<float> radiusStaging;

  float edgeWidthPx = 2.0f;
  float nodeColor[4] = {0.85f, 0.85f, 0.95f, 1.0f};
  float edgeColor[4] = {0.45f, 0.5f, 0.6f, 0.8f};
  float bgColor[4] = {0.08f, 0.09f, 0.11f, 1.0f};

  OrbitCamera camera;

  bool haveLastCursor = false;
  double lastCursorX = 0.0;
  double lastCursorY = 0.0;
  bool orbitDragging = false; // left button: 3D orbit, no-op in 2D
  bool panDragging = false;   // right/middle button: pan, both 2D and 3D

  float bboxMin[3] = {0.0f, 0.0f, 0.0f};
  float bboxMax[3] = {0.0f, 0.0f, 0.0f};
  bool bboxValid = false;

  ~Impl();

  bool Init(uint32_t width, uint32_t height, const std::string &title);
  bool CreatePipelines();
  bool CreateDepthTexture(uint32_t width, uint32_t height);
  WGPUBuffer CreateBuffer(size_t size, WGPUBufferUsage usage,
                          const char *label);
  bool EnsureBuffer(WGPUBuffer &buf, size_t &capacity, size_t neededBytes,
                    WGPUBufferUsage usage, const char *label);
  void RebuildBindGroup();
  void RecomputeFrameData(Graph &graph);
  void FrameCameraIfNeeded();
  void ComputeViewProj(float outMat[16], uint32_t fbw, uint32_t fbh) const;

  static void OnMouseButton(GLFWwindow *window, int button, int action,
                            int mods);
  static void OnCursorPos(GLFWwindow *window, double x, double y);
  static void OnScroll(GLFWwindow *window, double xoffset, double yoffset);
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

void Renderer::Impl::RebuildBindGroup() {
  if (bindGroup)
    wgpuBindGroupRelease(bindGroup);

  WGPUBindGroupEntry entries[5] = {};
  entries[0] = WGPUBindGroupEntry{
      .binding = 0, .buffer = globalsBuf, .size = sizeof(GlobalsUBO)};
  entries[1] = WGPUBindGroupEntry{
      .binding = 1, .buffer = positionsBuf, .size = positionsCapacity};
  entries[2] = WGPUBindGroupEntry{
      .binding = 2, .buffer = nodeIdsBuf, .size = nodeIdsCapacity};
  entries[3] = WGPUBindGroupEntry{
      .binding = 3, .buffer = edgesBuf, .size = edgesCapacity};
  entries[4] = WGPUBindGroupEntry{
      .binding = 4, .buffer = radiusBuf, .size = radiusCapacity};

  bindGroup = wgpuDeviceCreateBindGroup(
      device, WgpuPtr(WGPUBindGroupDescriptor{
                  .label = {"render bind group", WGPU_STRLEN},
                  .layout = bindGroupLayout,
                  .entryCount = 5,
                  .entries = entries,
              }));
  bindGroupDirty = false;
}

void Renderer::Impl::RecomputeFrameData(Graph &graph) {
  positionsStaging.clear();
  nodeIdsStaging.clear();
  edgesStaging.clear();
  radiusStaging.clear();

  auto *posPool = graph.NodeSpace().GetPool<PositionComponent>();
  if (!posPool)
    return;

  bool useDefaultRadius = false;
  auto *radiusPool = graph.NodeSpace().GetPool<RadiusComponent>();
  if (!radiusPool)
    useDefaultRadius = true;

  const VisibleNodesResource *visible =
      graph.HasResource<VisibleNodesResource>()
          ? graph.GetResource<VisibleNodesResource>()
          : nullptr;

  positionsStaging.reserve(graph.Size() * 3);
  radiusStaging.reserve(graph.Size());

  float minB[3] = {std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()};
  float maxB[3] = {std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::lowest()};
  bool anyVisible = false;

  for (uint32_t i = 0; i < graph.Size(); i++) {
    DenseNodeID denseID{i};

    const PositionComponent *c = posPool->Find(denseID.Raw());
    float x = c ? static_cast<float>(c->pos[0]) : 0.0f;
    float y = c ? static_cast<float>(c->pos[1]) : 0.0f;
    float z = (c && camera.is3D) ? static_cast<float>(c->pos[2]) : 0.0f;
    positionsStaging.push_back(x);
    positionsStaging.push_back(y);
    positionsStaging.push_back(z);

    radiusStaging.push_back(
        useDefaultRadius
            ? DEFAULT_RADIUS
            : static_cast<float>(radiusPool->Find(denseID.Raw())->radius));

    bool isVisible =
        visible ? (denseID.Raw() < visible->Size() && visible->Test(denseID))
                : true;
    if (isVisible) {
      nodeIdsStaging.push_back(denseID.Raw());
      anyVisible = true;
      minB[0] = std::min(minB[0], x);
      minB[1] = std::min(minB[1], y);
      minB[2] = std::min(minB[2], z);
      maxB[0] = std::max(maxB[0], x);
      maxB[1] = std::max(maxB[1], y);
      maxB[2] = std::max(maxB[2], z);
    }
  }

  if (anyVisible) {
    std::copy(minB, minB + 3, bboxMin);
    std::copy(maxB, maxB + 3, bboxMax);
    bboxValid = true;
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

void Renderer::Impl::FrameCameraIfNeeded() {
  if (camera.initialized || !bboxValid)
    return;

  float center[3];
  for (int i = 0; i < 3; i++)
    center[i] = (bboxMin[i] + bboxMax[i]) * 0.5f;
  std::copy(center, center + 3, camera.target);

  if (camera.is3D) {
    float ext[3];
    for (int i = 0; i < 3; i++)
      ext[i] = bboxMax[i] - bboxMin[i];
    float radius =
        0.5f * std::sqrt(ext[0] * ext[0] + ext[1] * ext[1] + ext[2] * ext[2]);
    radius = std::max(radius, 1e-3f);
    camera.distance = radius / std::sin(kFovY * 0.5f) * 1.15f;
  } else {
    float halfW = std::max((bboxMax[0] - bboxMin[0]) * 0.5f, 1e-3f) * 1.15f;
    float halfH = std::max((bboxMax[1] - bboxMin[1]) * 0.5f, 1e-3f) * 1.15f;
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
    OrthoViewProj(outMat, camera.target[0], camera.target[1], halfW, halfH);
    return;
  }

  float eye[3];
  camera.Eye(eye);
  const float up[3] = {0.0f, 1.0f, 0.0f};
  float view[16];
  LookAtRH(view, eye, camera.target, up);

  float nearZ = std::max(0.01f, camera.distance * 0.01f);
  float farZ = std::max(nearZ * 10.0f, camera.distance * 100.0f);
  float proj[16];
  PerspectiveProj(proj, kFovY, aspect, nearZ, farZ);

  Mat4Multiply(outMat, proj, view);
}

namespace {
constexpr float kOrbitSensitivity = 0.006f; // radians per pixel
} // namespace

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

Renderer::Renderer(uint32_t width, uint32_t height, const std::string &title,
                   Graph &graph)
    : m_impl(std::make_unique<Impl>()) {
  DimensionResource *dim = graph.GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_impl->camera.is3D = *dim == DimensionResource::D3;

  if (!m_impl->Init(width, height, title)) {
    std::fprintf(stderr, "[render] Renderer initialization failed\n");
  }
}

Renderer::~Renderer() = default;

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
    r.CreateDepthTexture(r.surfaceConfig.width, r.surfaceConfig.height);
  }

  r.RecomputeFrameData(graph);

  size_t posBytes =
      std::max<size_t>(r.positionsStaging.size() * sizeof(float), 1);
  size_t nodeIdBytes =
      std::max<size_t>(r.nodeIdsStaging.size() * sizeof(uint32_t), 1);
  size_t edgeBytes =
      std::max<size_t>(r.edgesStaging.size() * sizeof(uint32_t), 1);
  size_t radiusBytes =
      std::max<size_t>(r.radiusStaging.size() * sizeof(float), 1);

  r.EnsureBuffer(r.positionsBuf, r.positionsCapacity, posBytes,
                 WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                 "render positions");
  r.EnsureBuffer(r.nodeIdsBuf, r.nodeIdsCapacity, nodeIdBytes,
                 WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                 "render node ids");
  r.EnsureBuffer(r.edgesBuf, r.edgesCapacity, edgeBytes,
                 WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                 "render edges");
  r.EnsureBuffer(r.radiusBuf, r.radiusCapacity, radiusBytes,
                 WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
                 "render radius");

  if (!r.positionsStaging.empty())
    wgpuQueueWriteBuffer(r.queue, r.positionsBuf, 0, r.positionsStaging.data(),
                         r.positionsStaging.size() * sizeof(float));
  if (!r.nodeIdsStaging.empty())
    wgpuQueueWriteBuffer(r.queue, r.nodeIdsBuf, 0, r.nodeIdsStaging.data(),
                         r.nodeIdsStaging.size() * sizeof(uint32_t));
  if (!r.edgesStaging.empty())
    wgpuQueueWriteBuffer(r.queue, r.edgesBuf, 0, r.edgesStaging.data(),
                         r.edgesStaging.size() * sizeof(uint32_t));
  if (!r.radiusStaging.empty()) {
    wgpuQueueWriteBuffer(r.queue, r.radiusBuf, 0, r.radiusStaging.data(),
                         r.radiusStaging.size() * sizeof(float));
  }

  if (r.bindGroupDirty)
    r.RebuildBindGroup();

  r.FrameCameraIfNeeded();

  GlobalsUBO globals{};
  r.ComputeViewProj(globals.viewProj, static_cast<uint32_t>(fbw),
                    static_cast<uint32_t>(fbh));
  globals.viewport[0] = static_cast<float>(fbw);
  globals.viewport[1] = static_cast<float>(fbh);
  globals.edgeWidthPx = r.edgeWidthPx;
  globals.worldPerPixel = r.camera.WorldPerPixel(static_cast<float>(fbh));
  r.camera.Basis(globals.cameraRight, globals.cameraUp);
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

  WGPUTextureView frame =
      wgpuTextureCreateView(surfaceTexture.texture, nullptr);
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(
      r.device,
      WgpuPtr(WGPUCommandEncoderDescriptor{.label = {"render", WGPU_STRLEN}}));

  WGPURenderPassColorAttachment colorAttachment{
      .view = frame,
      .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
      .loadOp = WGPULoadOp_Clear,
      .storeOp = WGPUStoreOp_Store,
      .clearValue = {r.bgColor[0], r.bgColor[1], r.bgColor[2], r.bgColor[3]},
  };
  WGPURenderPassDepthStencilAttachment depthAttachment{
      .view = r.depthView,
      .depthLoadOp = WGPULoadOp_Clear,
      .depthStoreOp = WGPUStoreOp_Store,
      .depthClearValue = 1.0f,
  };
  WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(
      encoder, WgpuPtr(WGPURenderPassDescriptor{
                   .label = {"render pass", WGPU_STRLEN},
                   .colorAttachmentCount = 1,
                   .colorAttachments = &colorAttachment,
                   .depthStencilAttachment = &depthAttachment,
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
