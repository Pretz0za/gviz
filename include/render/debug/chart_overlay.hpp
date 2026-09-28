#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "debug/chart_recorder.hpp"
#include "render/debug/chart_window.hpp"
#include <webgpu/webgpu.h>

typedef struct GLFWwindow GLFWwindow;

class ChartOverlay {
public:
  ~ChartOverlay();

  bool Init(GLFWwindow *window, WGPUDevice device,
            WGPUTextureFormat colorFormat, WGPUTextureFormat depthFormat);
  void Shutdown();

  void SetRecorder(const ChartRecorder *recorder);

  void NewFrame();
  void Draw(WGPURenderPassEncoder pass);

  ChartWindow &Charts();

private:
  ChartWindow m_charts;
  const ChartRecorder *m_recorder = nullptr;
  bool m_open = true;
  bool m_initialized = false;
};

#endif
