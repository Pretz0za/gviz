#include "render/debug/config_window.hpp"

#ifdef GVIZ_DEBUG_CHARTS

#include <cstdint>
#include <imgui.h>

namespace {

void RenderField(ConfigField &field) {
  switch (field.type) {
  case ConfigFieldType::Double: {
    double min = field.min, max = field.max;
    ImGui::SliderScalar(field.name.c_str(), ImGuiDataType_Double, field.value, &min, &max);
    break;
  }
  case ConfigFieldType::Float: {
    float min = static_cast<float>(field.min), max = static_cast<float>(field.max);
    ImGui::SliderScalar(field.name.c_str(), ImGuiDataType_Float, field.value, &min, &max);
    break;
  }
  case ConfigFieldType::Int32: {
    int32_t min = static_cast<int32_t>(field.min), max = static_cast<int32_t>(field.max);
    ImGui::SliderScalar(field.name.c_str(), ImGuiDataType_S32, field.value, &min, &max);
    break;
  }
  case ConfigFieldType::UInt32: {
    uint32_t min = static_cast<uint32_t>(field.min), max = static_cast<uint32_t>(field.max);
    ImGui::SliderScalar(field.name.c_str(), ImGuiDataType_U32, field.value, &min, &max);
    break;
  }
  case ConfigFieldType::UInt8: {
    uint8_t min = static_cast<uint8_t>(field.min), max = static_cast<uint8_t>(field.max);
    ImGui::SliderScalar(field.name.c_str(), ImGuiDataType_U8, field.value, &min, &max);
    break;
  }
  case ConfigFieldType::Bool:
    ImGui::Checkbox(field.name.c_str(), static_cast<bool *>(field.value));
    break;
  }
}

} // namespace

void ConfigWindow::Add(std::string label, IConfiguration &config) {
  m_entries.push_back({std::move(label), &config});
}

void ConfigWindow::Render(bool &open) {
  if (!open)
    return;
  if (!ImGui::Begin("Configuration", &open)) {
    ImGui::End();
    return;
  }

  for (Entry &entry : m_entries) {
    ImGui::SeparatorText(entry.label.c_str());
    for (ConfigField &field : entry.config->Fields())
      RenderField(field);
  }

  ImGui::End();
}

#endif
