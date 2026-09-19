#pragma once

#include "render/renderer.hpp"

template <GraphLike G>
Renderer::Renderer(uint32_t width, uint32_t height, const std::string &title,
                   G &graph)
    : Renderer(width, height, title, ReadIs3D(graph)) {}

template <GraphLike G> bool Renderer::Frame(G &graph) {
  FrameData &fd = FrameBuffer();
  ExtractFrameData(graph, Is3D(), fd);
  return ApplyFrame(fd);
}
