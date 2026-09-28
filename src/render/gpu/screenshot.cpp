#include "renderer_impl.hpp"

#include <webgpu/wgpu.h>

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

uint32_t AlignUp(uint32_t value, uint32_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

void PutU32(uint8_t *dst, uint32_t v) {
  dst[0] = static_cast<uint8_t>(v);
  dst[1] = static_cast<uint8_t>(v >> 8);
  dst[2] = static_cast<uint8_t>(v >> 16);
  dst[3] = static_cast<uint8_t>(v >> 24);
}

void WriteBmp(const std::string &path, uint32_t width, uint32_t height,
             const uint8_t *pixels, uint32_t srcRowStride,
             bool sourceIsBGRA) {
  uint32_t dstRowStride = AlignUp(width * 3, 4);
  uint32_t pixelDataSize = dstRowStride * height;
  uint32_t fileSize = 54 + pixelDataSize;

  uint8_t fileHeader[14] = {'B', 'M'};
  PutU32(fileHeader + 2, fileSize);
  PutU32(fileHeader + 10, 54);

  uint8_t infoHeader[40] = {};
  PutU32(infoHeader + 0, 40);
  PutU32(infoHeader + 4, width);
  PutU32(infoHeader + 8, static_cast<uint32_t>(-static_cast<int32_t>(height)));
  infoHeader[12] = 1;
  infoHeader[14] = 24;
  PutU32(infoHeader + 20, pixelDataSize);

  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f) {
    std::fprintf(stderr, "[render] failed to open %s for screenshot\n",
                path.c_str());
    return;
  }
  std::fwrite(fileHeader, 1, sizeof(fileHeader), f);
  std::fwrite(infoHeader, 1, sizeof(infoHeader), f);

  std::vector<uint8_t> row(dstRowStride, 0);
  for (uint32_t y = 0; y < height; y++) {
    const uint8_t *srcRow = pixels + static_cast<size_t>(y) * srcRowStride;
    for (uint32_t x = 0; x < width; x++) {
      const uint8_t *texel = srcRow + x * 4;
      uint8_t r, g, b;
      if (sourceIsBGRA) {
        b = texel[0];
        g = texel[1];
        r = texel[2];
      } else {
        r = texel[0];
        g = texel[1];
        b = texel[2];
      }
      row[x * 3 + 0] = b;
      row[x * 3 + 1] = g;
      row[x * 3 + 2] = r;
    }
    std::fwrite(row.data(), 1, dstRowStride, f);
  }
  std::fclose(f);
}

struct MapResult {
  bool done = false;
  bool ok = false;
};

void OnBufferMapped(WGPUMapAsyncStatus status, WGPUStringView, void *userdata1,
                    void *) {
  auto *result = static_cast<MapResult *>(userdata1);
  result->done = true;
  result->ok = status == WGPUMapAsyncStatus_Success;
}

}

void Renderer::Impl::EncodeScreenshotCopy(WGPUCommandEncoder encoder,
                                          WGPUTexture source, uint32_t fbw,
                                          uint32_t fbh) {
  if (pendingScreenshotPath.empty())
    return;
  if (!screenshotCopySupported) {
    std::fprintf(stderr,
                 "[render] screenshot requested but this surface does not "
                 "support CopySrc; skipping\n");
    pendingScreenshotPath.clear();
    return;
  }

  uint32_t rowStride = AlignUp(fbw * 4, 256);
  size_t bufSize = static_cast<size_t>(rowStride) * fbh;
  if (!EnsureBuffer(screenshotBuf, screenshotBufCapacity, bufSize,
                    WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst,
                    "screenshot readback"))
    return;

  WGPUTexelCopyTextureInfo copySrc{
      .texture = source,
      .mipLevel = 0,
      .origin = {0, 0, 0},
      .aspect = WGPUTextureAspect_All,
  };
  WGPUTexelCopyBufferInfo copyDst{
      .layout = {.offset = 0, .bytesPerRow = rowStride, .rowsPerImage = fbh},
      .buffer = screenshotBuf,
  };
  WGPUExtent3D extent{fbw, fbh, 1};
  wgpuCommandEncoderCopyTextureToBuffer(encoder, &copySrc, &copyDst, &extent);
}

void Renderer::Impl::WriteScreenshot(uint32_t fbw, uint32_t fbh) {
  if (pendingScreenshotPath.empty() || !screenshotCopySupported)
    return;

  uint32_t rowStride = AlignUp(fbw * 4, 256);
  size_t bufSize = static_cast<size_t>(rowStride) * fbh;

  wgpuDevicePoll(device, true, nullptr);

  MapResult result;
  WGPUBufferMapCallbackInfo callbackInfo{
      .mode = WGPUCallbackMode_AllowProcessEvents,
      .callback = OnBufferMapped,
      .userdata1 = &result,
  };
  wgpuBufferMapAsync(screenshotBuf, WGPUMapMode_Read, 0, bufSize,
                     callbackInfo);
  while (!result.done)
    wgpuDevicePoll(device, true, nullptr);

  if (!result.ok) {
    std::fprintf(stderr, "[render] failed to map screenshot buffer\n");
    pendingScreenshotPath.clear();
    return;
  }

  const auto *mapped = static_cast<const uint8_t *>(
      wgpuBufferGetConstMappedRange(screenshotBuf, 0, bufSize));
  bool sourceIsBGRA = surfaceFormat == WGPUTextureFormat_BGRA8Unorm;
  WriteBmp(pendingScreenshotPath, fbw, fbh, mapped, rowStride, sourceIsBGRA);
  std::fprintf(stderr, "[render] wrote screenshot to %s\n",
              pendingScreenshotPath.c_str());

  wgpuBufferUnmap(screenshotBuf);
  pendingScreenshotPath.clear();
}
