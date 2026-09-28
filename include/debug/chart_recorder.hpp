#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "debug/chart_series.hpp"
#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>

class ChartRecorder {
public:
  explicit ChartRecorder(IndexSpace &space) : m_space(space) {}

  template <typename T> inline void PushFrame();

  template <typename T> inline const std::vector<std::vector<T>> &Samples() const;
  template <typename T> inline size_t SampleCount() const;

private:
  template <typename T> inline ChartSeries<T> &Series();

  IndexSpace &m_space;
  std::unordered_map<std::type_index, std::unique_ptr<IChartSeries>> m_series;
};

struct ChartRecorderResource {
  std::unique_ptr<ChartRecorder> recorder;

  explicit ChartRecorderResource(IndexSpace &space)
      : recorder(std::make_unique<ChartRecorder>(space)) {}
};

#include "debug/chart_recorder.tpp"

#endif
