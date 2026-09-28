#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "debug/chart_recorder.hpp"

template <typename T> void ChartRecorder::PushFrame() {
  DenseComponentPool<T> *pool = m_space.GetPool<T>();
  if (!pool)
    return;
  Series<T>().PushFrame(pool->Data());
}

template <typename T>
const std::vector<std::vector<T>> &ChartRecorder::Samples() const {
  return const_cast<ChartRecorder *>(this)->Series<T>().Samples();
}

template <typename T> size_t ChartRecorder::SampleCount() const {
  auto it = m_series.find(std::type_index(typeid(T)));
  return it == m_series.end() ? 0 : it->second->SampleCount();
}

template <typename T> ChartSeries<T> &ChartRecorder::Series() {
  auto key = std::type_index(typeid(T));
  auto it = m_series.find(key);
  if (it == m_series.end()) {
    auto series = std::make_unique<ChartSeries<T>>();
    auto *raw = series.get();
    m_series.emplace(key, std::move(series));
    return *raw;
  }
  return *static_cast<ChartSeries<T> *>(it->second.get());
}

#endif
