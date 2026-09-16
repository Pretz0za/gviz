#pragma once

#include "graph/graph.hpp"
#include <cstdint>
#include <memory>
#include <string>

class Renderer {
public:
  Renderer(uint32_t width, uint32_t height, const std::string &title);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  bool Frame(Graph &graph);

  void SetNodeRadiusPixels(float radius);
  void SetEdgeWidthPixels(float width);
  void SetNodeColor(float r, float g, float b, float a);
  void SetEdgeColor(float r, float g, float b, float a);
  void SetBackgroundColor(float r, float g, float b, float a);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
