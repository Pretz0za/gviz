#pragma once

#ifdef GVIZ_DEBUG_CHARTS

#include "debug/chart_recorder.hpp"
#include "render/debug/chart_sample.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class IChartTrack {
public:
  virtual ~IChartTrack() = default;
  virtual void Sync(const ChartRecorder &recorder) = 0;
  virtual void Render() const = 0;
  virtual const std::string &Label() const = 0;
};

template <typename T> class ChartTrack : public IChartTrack {
public:
  using DecodeFn = std::function<std::vector<ChartDataPoint>(const std::vector<T> &)>;

  ChartTrack(std::string label, DecodeFn decode)
      : m_label(std::move(label)), m_decode(std::move(decode)) {}

  void Sync(const ChartRecorder &recorder) override;
  void Render() const override;
  const std::string &Label() const override { return m_label; }

private:
  std::string m_label;
  DecodeFn m_decode;
  size_t m_processed = 0;
  std::vector<std::string> m_seriesOrder;
  std::unordered_map<std::string, std::vector<float>> m_series;
};

#include "render/debug/chart_track.tpp"

#endif
