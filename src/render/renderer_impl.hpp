#pragma once

#include "camera/camera.hpp"
#include "gpu/render_internal.hpp"
#include "render/renderer.hpp"

#include <webgpu/webgpu.h>

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

  FrameData frameData;

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

  ~Impl();

  bool Init(uint32_t width, uint32_t height, const std::string &title);
  bool CreatePipelines();
  bool CreateDepthTexture(uint32_t width, uint32_t height);
  WGPUBuffer CreateBuffer(size_t size, WGPUBufferUsage usage,
                          const char *label);
  bool EnsureBuffer(WGPUBuffer &buf, size_t &capacity, size_t neededBytes,
                    WGPUBufferUsage usage, const char *label);
  void RebuildBindGroup();
  bool ApplyFrame(const FrameData &fd);
  void FrameCameraIfNeeded(const FrameData &fd);
  void ComputeViewProj(float outMat[16], uint32_t fbw, uint32_t fbh) const;

  static void OnMouseButton(GLFWwindow *window, int button, int action,
                            int mods);
  static void OnCursorPos(GLFWwindow *window, double x, double y);
  static void OnScroll(GLFWwindow *window, double xoffset, double yoffset);
};
