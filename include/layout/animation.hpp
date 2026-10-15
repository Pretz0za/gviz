#pragma once

#include "concept/graphLike.hpp"
#include "layout/components/animation_position.hpp"
#include "layout/components/position.hpp"
#include <chrono>
#include <cmath>
#include <cstdint>

enum class Easing : uint8_t { Linear, EaseInOutCubic };

inline double ApplyEasing(Easing easing, double t) {
  switch (easing) {
  case Easing::EaseInOutCubic:
    return t < 0.5 ? 4.0 * t * t * t
                   : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;
  case Easing::Linear:
  default:
    return t;
  }
}

template <GraphLike G> class PositionAnimationSystem {
public:
  PositionAnimationSystem() = default;

  void Init(G &graph);

  void Begin();

  void End(double durationSeconds, Easing easing = Easing::EaseInOutCubic);

  bool Advance();

  bool Active() const { return m_active; }

private:
  double CurrentAlpha() const;

  DenseComponentPool<PositionComponent> *m_position = nullptr;
  DenseComponentPool<AnimationFromPositionComponent> *m_from = nullptr;
  DenseComponentPool<AnimationToPositionComponent> *m_to = nullptr;
  Easing m_easing = Easing::Linear;
  double m_duration = 0.0;
  bool m_active = false;
  std::chrono::steady_clock::time_point m_start;
};

#include "layout/animation.tpp"
