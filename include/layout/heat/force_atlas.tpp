#pragma once

#include "ds/vector.hpp"
#include "ecs/exceptions.hpp"
#include "layout/heat/force_atlas.hpp"
#include "layout/types.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr double kForceAtlasEpsilon = 1e-7;
constexpr double kSpeedEfficiencyMin = 0.05;
constexpr double kSpeedEfficiencyMax = 1.0;
constexpr double kSpeedMaxRise = 0.5;
} // namespace

template <GraphLike G>
ForceAtlasHeat<G>::ForceAtlasHeat(G &graph)
    : m_heatPool(
          graph.NodeSpace().template GetPool<ForceAtlasHeatComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G> void ForceAtlasHeat<G>::BeginTick() {
  m_totalSwinging = 0.0;
  m_totalTraction = 0.0;
}

template <GraphLike G>
void ForceAtlasHeat<G>::Observe(DenseNodeID id, const double *force) {
  ForceAtlasHeatComponent *heat = &m_heatPool->Data()[id.Raw()];

  double diff[m_dimension];
  double sum[m_dimension];
  Subtract(force, heat->oldForce, diff, m_dimension);
  Copy(force, sum, m_dimension);
  Vecaxpy(1.0, heat->oldForce, sum, m_dimension);

  heat->swinging = L2Norm(diff, m_dimension);
  double traction = 0.5 * L2Norm(sum, m_dimension);

  m_totalSwinging += heat->swinging;
  m_totalTraction += traction;

  Copy(force, heat->oldForce, m_dimension);
}

template <GraphLike G>
void ForceAtlasHeat<G>::UpdateGlobalSpeed(uint32_t nodeCount) {
  if (m_totalTraction < kForceAtlasEpsilon)
    return;

  double n = static_cast<double>(nodeCount);
  double estimatedOptimalJT = 0.05 * std::sqrt(n);
  double minJT = std::sqrt(estimatedOptimalJT);
  double maxJT = 10.0;
  double jt =
      m_jitterTolerance *
      std::max(minJT, std::min(maxJT, estimatedOptimalJT * m_totalTraction /
                                          (n * n)));

  if (m_totalSwinging / m_totalTraction > 2.0) {
    if (m_speedEfficiency > kSpeedEfficiencyMin)
      m_speedEfficiency *= 0.5;
    jt = std::max(jt, m_jitterTolerance);
  }

  double targetSpeed = m_totalSwinging > kForceAtlasEpsilon
                           ? jt * m_speedEfficiency * m_totalTraction /
                                 m_totalSwinging
                           : m_globalSpeed;

  if (m_totalSwinging > jt * m_totalTraction) {
    if (m_speedEfficiency > kSpeedEfficiencyMin)
      m_speedEfficiency *= 0.7;
  } else if (m_globalSpeed < 1000.0) {
    m_speedEfficiency *= 1.3;
  }

  m_speedEfficiency =
      std::clamp(m_speedEfficiency, kSpeedEfficiencyMin, kSpeedEfficiencyMax);
  m_globalSpeed +=
      std::min(targetSpeed - m_globalSpeed, kSpeedMaxRise * m_globalSpeed);
}

template <GraphLike G>
double ForceAtlasHeat<G>::SpeedFactor(DenseNodeID id) const {
  double swinging = m_heatPool->Find(id.Raw())->swinging;
  return m_globalSpeed / (1.0 + std::sqrt(m_globalSpeed * swinging));
}
