#pragma once

#include "render/graph_query.hpp"
#include <cstdint>
#include <memory>
#include <string>

class Renderer {
public:
  template <GraphLike G>
  Renderer(uint32_t width, uint32_t height, const std::string &title,
           G &graph);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  template <GraphLike G> bool Frame(G &graph);

  void SetEdgeWidthPixels(float width);
  void SetNodeColor(float r, float g, float b, float a);
  void SetEdgeColor(float r, float g, float b, float a);
  void SetBackgroundColor(float r, float g, float b, float a);

private:
  Renderer(uint32_t width, uint32_t height, const std::string &title,
           bool is3D);

  bool Is3D() const;
  FrameData &FrameBuffer();
  bool ApplyFrame(const FrameData &fd);

  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

#include "render/renderer.tpp"
