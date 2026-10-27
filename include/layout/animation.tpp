#pragma once

#include "layout/animation.hpp"
#include <algorithm>

template <GraphLike G> void PositionAnimationSystem<G>::Init(DecoratedGraph<G> &dg) {
  m_position = dg.nodeSpace.template GetPool<PositionComponent>();
  uint8_t dimension = m_position->Dimension();
  m_from = dg.nodeSpace
               .template SetPool<AnimationFromPositionComponent>(dimension);
  m_to = dg.nodeSpace
             .template SetPool<AnimationToPositionComponent>(dimension);
}

template <GraphLike G> double PositionAnimationSystem<G>::CurrentAlpha() const {
  if (!m_active)
    return 1.0;
  double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                                  m_start)
                       .count();
  double t = m_duration > 0.0 ? elapsed / m_duration : 1.0;
  return ApplyEasing(m_easing, std::clamp(t, 0.0, 1.0));
}

template <GraphLike G> void PositionAnimationSystem<G>::Begin() {
  uint8_t dim = m_position->Dimension();
  auto &pos = m_position->Data();
  auto &from = m_from->Data();
  auto &to = m_to->Data();
  double alpha = CurrentAlpha();
  for (size_t i = 0; i < pos.Size(); i++) {
    for (uint8_t d = 0; d < dim; d++) {
      double f = from[i].pos[d];
      double t = m_active ? to[i].pos[d] : pos[i].pos[d];
      from[i].pos[d] = f + (t - f) * alpha;
    }
  }
  m_active = false;
}

template <GraphLike G>
void PositionAnimationSystem<G>::End(double durationSeconds, Easing easing) {
  uint8_t dim = m_position->Dimension();
  auto &pos = m_position->Data();
  auto &from = m_from->Data();
  auto &to = m_to->Data();
  for (size_t i = 0; i < pos.Size(); i++) {
    for (uint8_t d = 0; d < dim; d++) {
      to[i].pos[d] = pos[i].pos[d];
      pos[i].pos[d] = from[i].pos[d];
    }
  }
  m_duration = durationSeconds;
  m_easing = easing;
  m_start = std::chrono::steady_clock::now();
  m_active = durationSeconds > 0.0;
}

template <GraphLike G> bool PositionAnimationSystem<G>::Advance() {
  if (!m_active)
    return false;
  double alpha = CurrentAlpha();
  uint8_t dim = m_position->Dimension();
  auto &pos = m_position->Data();
  auto &from = m_from->Data();
  auto &to = m_to->Data();
  for (size_t i = 0; i < pos.Size(); i++)
    for (uint8_t d = 0; d < dim; d++)
      pos[i].pos[d] = from[i].pos[d] + (to[i].pos[d] - from[i].pos[d]) * alpha;
  if (alpha >= 1.0)
    m_active = false;
  return m_active;
}
