#pragma once

#include "render/renderer.hpp"

template <GraphLike G>
Renderer::Renderer(uint32_t width, uint32_t height, const std::string &title,
                   DecoratedGraph<G> &dg)
    : Renderer(width, height, title, ReadIs3D(dg)) {}

template <GraphLike G> bool Renderer::Frame(DecoratedGraph<G> &dg) {
  FrameData &fd = FrameBuffer();
  RenderQuerySystem<G> query(dg);
  query.ExtractFrameData(Is3D(), fd);
#ifdef GVIZ_DEBUG_CHARTS
  SyncChartRecorder(dg.template GetResource<ChartRecorderResource>());
#endif
  return ApplyFrame(fd);
}
