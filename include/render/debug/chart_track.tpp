#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "render/debug/chart_track.hpp"
#include <cfloat>
#include <imgui.h>

template <typename T> void ChartTrack<T>::Sync(const ChartRecorder &recorder) {
  size_t total = recorder.SampleCount<T>();
  if (total <= m_processed)
    return;

  const std::vector<std::vector<T>> &frames = recorder.Samples<T>();
  for (size_t i = m_processed; i < total; i++) {
    for (const ChartDataPoint &point : m_decode(frames[i])) {
      auto &series = m_series[point.name];
      if (series.empty())
        m_seriesOrder.push_back(point.name);
      series.push_back(static_cast<float>(point.value));
    }
  }
  m_processed = total;
}

template <typename T> void ChartTrack<T>::Render() const {
  for (const std::string &name : m_seriesOrder) {
    const std::vector<float> &values = m_series.at(name);
    if (values.empty())
      continue;
    std::string title = m_label + " / " + name;
    ImGui::TextUnformatted(title.c_str());
    ImGui::PlotLines(("##" + title).c_str(), values.data(),
                      static_cast<int>(values.size()), 0, nullptr, FLT_MAX,
                      FLT_MAX, ImVec2(0, 80));
  }
}

#endif
