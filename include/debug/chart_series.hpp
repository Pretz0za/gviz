#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include <cstddef>
#include <vector>

class IChartSeries {
public:
  virtual ~IChartSeries() = default;
  virtual size_t SampleCount() const = 0;
};

template <typename T> class ChartSeries : public IChartSeries {
public:
  inline void PushFrame(std::vector<T> sample);

  inline size_t SampleCount() const override;

  inline const std::vector<std::vector<T>> &Samples() const;

private:
  std::vector<std::vector<T>> m_samples;
};

#include "debug/chart_series.tpp"

#endif
