#include "render/debug/chart_overlay.hpp"

#ifdef GVIZ_DEBUG_CHARTS

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>

ChartOverlay::~ChartOverlay() { Shutdown(); }

bool ChartOverlay::Init(GLFWwindow *window, WGPUDevice device,
                        WGPUTextureFormat colorFormat,
                        WGPUTextureFormat depthFormat) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;

  if (!ImGui_ImplGlfw_InitForOther(window, true)) {
    ImGui::DestroyContext();
    return false;
  }

  ImGui_ImplWGPU_InitInfo initInfo{};
  initInfo.Device = device;
  initInfo.NumFramesInFlight = 3;
  initInfo.RenderTargetFormat = colorFormat;
  initInfo.DepthStencilFormat = depthFormat;
  if (!ImGui_ImplWGPU_Init(&initInfo)) {
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return false;
  }

  m_initialized = true;
  return true;
}

void ChartOverlay::Shutdown() {
  if (!m_initialized)
    return;
  ImGui_ImplWGPU_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  m_initialized = false;
}

void ChartOverlay::SetRecorder(const ChartRecorder *recorder) {
  m_recorder = recorder;
}

void ChartOverlay::NewFrame() {
  if (!m_initialized)
    return;

  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  if (m_recorder)
    m_charts.Render(*m_recorder, m_open);
  ImGui::Render();
}

void ChartOverlay::Draw(WGPURenderPassEncoder pass) {
  if (!m_initialized)
    return;
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
}

ChartWindow &ChartOverlay::Charts() { return m_charts; }

#endif
