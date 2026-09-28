#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "debug/chart_series.hpp"

template <typename T> void ChartSeries<T>::PushFrame(std::vector<T> sample) {
  m_samples.push_back(std::move(sample));
}

template <typename T> size_t ChartSeries<T>::SampleCount() const {
  return m_samples.size();
}

template <typename T>
const std::vector<std::vector<T>> &ChartSeries<T>::Samples() const {
  return m_samples;
}

#endif
