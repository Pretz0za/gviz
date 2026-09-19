#include "renderer_impl.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>

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

bool Renderer::Impl::ApplyFrame(const FrameData &fd) {
  if (!window || glfwWindowShouldClose(window))
    return false;

  glfwPollEvents();

  int fbw, fbh;
  glfwGetFramebufferSize(window, &fbw, &fbh);
  if (fbw == 0 || fbh == 0)
    return true;
  if (static_cast<uint32_t>(fbw) != surfaceConfig.width ||
      static_cast<uint32_t>(fbh) != surfaceConfig.height) {
    surfaceConfig.width = static_cast<uint32_t>(fbw);
    surfaceConfig.height = static_cast<uint32_t>(fbh);
    wgpuSurfaceConfigure(surface, &surfaceConfig);
    CreateDepthTexture(surfaceConfig.width, surfaceConfig.height);
  }

  size_t posBytes = std::max<size_t>(fd.positions.size() * sizeof(float), 1);
  size_t nodeIdBytes =
      std::max<size_t>(fd.nodeIds.size() * sizeof(uint32_t), 1);
  size_t edgeBytes = std::max<size_t>(fd.edges.size() * sizeof(uint32_t), 1);
  size_t radiusBytes = std::max<size_t>(fd.radii.size() * sizeof(float), 1);

  EnsureBuffer(positionsBuf, positionsCapacity, posBytes,
              WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
              "render positions");
  EnsureBuffer(nodeIdsBuf, nodeIdsCapacity, nodeIdBytes,
              WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
              "render node ids");
  EnsureBuffer(edgesBuf, edgesCapacity, edgeBytes,
              WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
              "render edges");
  EnsureBuffer(radiusBuf, radiusCapacity, radiusBytes,
              WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst,
              "render radius");

  if (!fd.positions.empty())
    wgpuQueueWriteBuffer(queue, positionsBuf, 0, fd.positions.data(),
                         fd.positions.size() * sizeof(float));
  if (!fd.nodeIds.empty())
    wgpuQueueWriteBuffer(queue, nodeIdsBuf, 0, fd.nodeIds.data(),
                         fd.nodeIds.size() * sizeof(uint32_t));
  if (!fd.edges.empty())
    wgpuQueueWriteBuffer(queue, edgesBuf, 0, fd.edges.data(),
                         fd.edges.size() * sizeof(uint32_t));
  if (!fd.radii.empty())
    wgpuQueueWriteBuffer(queue, radiusBuf, 0, fd.radii.data(),
                         fd.radii.size() * sizeof(float));

  if (bindGroupDirty)
    RebuildBindGroup();

  FrameCameraIfNeeded(fd);

  GlobalsUBO globals{};
  ComputeViewProj(globals.viewProj, static_cast<uint32_t>(fbw),
                 static_cast<uint32_t>(fbh));
  globals.viewport[0] = static_cast<float>(fbw);
  globals.viewport[1] = static_cast<float>(fbh);
  globals.edgeWidthPx = edgeWidthPx;
  globals.worldPerPixel = camera.WorldPerPixel(static_cast<float>(fbh));
  camera.Basis(globals.cameraRight, globals.cameraUp);
  std::memcpy(globals.nodeColor, nodeColor, sizeof(globals.nodeColor));
  std::memcpy(globals.edgeColor, edgeColor, sizeof(globals.edgeColor));
  wgpuQueueWriteBuffer(queue, globalsBuf, 0, &globals, sizeof(globals));

  WGPUSurfaceTexture surfaceTexture;
  wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
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
      device,
      WgpuPtr(WGPUCommandEncoderDescriptor{.label = {"render", WGPU_STRLEN}}));

  WGPURenderPassColorAttachment colorAttachment{
      .view = frame,
      .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
      .loadOp = WGPULoadOp_Clear,
      .storeOp = WGPUStoreOp_Store,
      .clearValue = {bgColor[0], bgColor[1], bgColor[2], bgColor[3]},
  };
  WGPURenderPassDepthStencilAttachment depthAttachment{
      .view = depthView,
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

  wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
  wgpuRenderPassEncoderSetPipeline(pass, edgePipeline);
  wgpuRenderPassEncoderDraw(pass, 6, static_cast<uint32_t>(fd.edges.size() / 2),
                           0, 0);
  wgpuRenderPassEncoderSetPipeline(pass, nodePipeline);
  wgpuRenderPassEncoderDraw(pass, 6, static_cast<uint32_t>(fd.nodeIds.size()),
                           0, 0);
  wgpuRenderPassEncoderEnd(pass);
  wgpuRenderPassEncoderRelease(pass);

  WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
  wgpuQueueSubmit(queue, 1, &commands);
  wgpuSurfacePresent(surface);

  wgpuCommandBufferRelease(commands);
  wgpuCommandEncoderRelease(encoder);
  wgpuTextureViewRelease(frame);
  wgpuTextureRelease(surfaceTexture.texture);
  return true;
}
