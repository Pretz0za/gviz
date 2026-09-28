#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "debug/chart_recorder.hpp"
#include "render/debug/chart_track.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

class ChartWindow {
public:
  template <typename T>
  void With(std::string label, typename ChartTrack<T>::DecodeFn decode);

  template <typename T> void Remove();

  void Clear();

  void Render(const ChartRecorder &recorder, bool &open);

private:
  std::vector<std::type_index> m_order;
  std::unordered_map<std::type_index, std::unique_ptr<IChartTrack>> m_tracks;
};

#include "render/debug/chart_window.tpp"

#endif
