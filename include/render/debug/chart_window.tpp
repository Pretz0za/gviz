#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "render/debug/chart_window.hpp"

template <typename T>
void ChartWindow::With(std::string label,
                       typename ChartTrack<T>::DecodeFn decode) {
  std::type_index key(typeid(T));
  if (!m_tracks.contains(key))
    m_order.push_back(key);
  m_tracks[key] = std::make_unique<ChartTrack<T>>(std::move(label), std::move(decode));
}

template <typename T> void ChartWindow::Remove() {
  std::type_index key(typeid(T));
  m_tracks.erase(key);
  std::erase(m_order, key);
}

#endif
