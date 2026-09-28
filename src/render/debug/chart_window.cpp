#include "render/debug/chart_window.hpp"

#ifdef GVIZ_DEBUG_CHARTS

#include <imgui.h>

void ChartWindow::Clear() {
  m_tracks.clear();
  m_order.clear();
}

void ChartWindow::Render(const ChartRecorder &recorder, bool &open) {
  if (!open)
    return;
  if (!ImGui::Begin("Debug Charts", &open)) {
    ImGui::End();
    return;
  }

  for (const std::type_index &key : m_order) {
    auto it = m_tracks.find(key);
    if (it == m_tracks.end())
      continue;
    IChartTrack &track = *it->second;
    track.Sync(recorder);
    ImGui::SeparatorText(track.Label().c_str());
    track.Render();
  }

  ImGui::End();
}

#endif
