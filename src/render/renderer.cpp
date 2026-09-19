#include "renderer_impl.hpp"

#include <cstdio>

Renderer::Renderer(uint32_t width, uint32_t height, const std::string &title,
                   bool is3D)
    : m_impl(std::make_unique<Impl>()) {
  m_impl->camera.is3D = is3D;
  if (!m_impl->Init(width, height, title)) {
    std::fprintf(stderr, "[render] Renderer initialization failed\n");
  }
}

Renderer::~Renderer() = default;

bool Renderer::Is3D() const { return m_impl->camera.is3D; }

FrameData &Renderer::FrameBuffer() { return m_impl->frameData; }

bool Renderer::ApplyFrame(const FrameData &fd) { return m_impl->ApplyFrame(fd); }

void Renderer::SetEdgeWidthPixels(float width) { m_impl->edgeWidthPx = width; }

void Renderer::SetNodeColor(float r, float g, float b, float a) {
  m_impl->nodeColor[0] = r;
  m_impl->nodeColor[1] = g;
  m_impl->nodeColor[2] = b;
  m_impl->nodeColor[3] = a;
}

void Renderer::SetEdgeColor(float r, float g, float b, float a) {
  m_impl->edgeColor[0] = r;
  m_impl->edgeColor[1] = g;
  m_impl->edgeColor[2] = b;
  m_impl->edgeColor[3] = a;
}

void Renderer::SetBackgroundColor(float r, float g, float b, float a) {
  m_impl->bgColor[0] = r;
  m_impl->bgColor[1] = g;
  m_impl->bgColor[2] = b;
  m_impl->bgColor[3] = a;
}
