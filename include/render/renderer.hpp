#pragma once

#include "graph/graph.hpp"
#include <cstdint>
#include <memory>
#include <string>

class Renderer {
public:
  // Reads DimensionResource from `graph` once, at construction, to decide
  // whether the camera runs in 2D (pan + zoom) or 3D (orbit + pan + zoom)
  // mode. Throws MissingResourceException<DimensionResource> if the graph
  // has no DimensionResource set.
  Renderer(uint32_t width, uint32_t height, const std::string &title,
           Graph &graph);
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
