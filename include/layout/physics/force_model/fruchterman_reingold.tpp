#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "graph/types.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"
#include <cmath>
#include <cstdint>

template <GraphLike G>
VanillaFruchtermanReingold<G>::VanillaFruchtermanReingold(G &graph)
    : m_distanceCalc(graph),
      m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()),
      m_positions(graph.NodeSpace().template GetPool<PositionComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::AttractiveTick(DenseNodeID v,
                                                   DenseNodeID u) {
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(v, u, out);
  double gap = m_distanceCalc.BetweenNodes(v, u);
  Vecaxpy(std::fabs(gap) / m_edgeLength, out, m_physics->Find(v.Raw())->disp,
          m_dimension);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v,
                                                  DenseNodeID u) {
  double out[m_dimension];
  // v - u (not u - v): repulsion pushes v away from u.
  m_distanceCalc.VecBetweenNodes(u, v, out);
  double dist = m_distanceCalc.BetweenNodes(v, u);

  // See kMinDistFraction: floor the magnitude's distance so a near-zero
  // (or negative, once overlapping) gap can't blow this up.
  double safeDist = std::fmax(std::fabs(dist), m_edgeLength * kMinDistFraction);
  Vecaxpy(m_edgeLength * m_edgeLength / (safeDist * safeDist), out,
          m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v, const double *uPos,
                                                  double uMass) {
  double out[m_dimension];
  // v - u (not u - v): repulsion pushes v away from u.
  //
  // PositionComponent::pos is declared double[1] but is really backed by
  // an m_dimension-wide slice of PositionSpan's shared storage, valid
  // only through a pointer/reference into that storage -- a by-value
  // PositionComponent copy (`auto vPos = *ptr`) only copies pos[0],
  // leaving every axis past X as uninitialized stack garbage. That
  // silently corrupted every repulsive force computed through this
  // overload (the Barnes-Hut hot path) on every axis but X.
  const PositionComponent *vPos = m_positions->Find(v.Raw());
  Subtract(vPos->pos, uPos, out, m_dimension);
  double dist = L2Norm(out, m_dimension);

  if (dist < 1e-9) {
    // Exactly (or numerically) coincident: there's no direction to push
    // in, so pick a stable, deterministic one from the node's own bit
    // pattern rather than leaving the pair stuck with a zero vector.
    uint64_t bits = reinterpret_cast<uintptr_t>(m_physics->Find(v.Raw()));
    bits ^= bits >> 33;
    bits *= 0x9E3779B97F4A7C15ULL;
    double angle = (bits >> 40) * (6.283185307179586 / (1ull << 24));
    out[0] = std::cos(angle);
    if (m_dimension > 1) out[1] = std::sin(angle);
    for (uint8_t d = 2; d < m_dimension; d++) out[d] = 0.0;
    dist = 1.0;
  }

  // See kMinDistFraction: floor the magnitude's distance so two points
  // landing arbitrarily close together can't turn k^2/dist into an
  // unbounded one-tick impulse (direction still follows the real,
  // unfloored `out`/dist).
  double safeDist = std::fmax(dist, m_edgeLength * kMinDistFraction);
  double mag = (m_edgeLength * m_edgeLength) / safeDist;
  Vecaxpy(mag / dist, out, m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
GRIPFruchtermanReingold<G>::GRIPFruchtermanReingold(G &graph)
    : m_distanceCalc(graph),
      m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G>
void GRIPFruchtermanReingold<G>::AttractiveTick(DenseNodeID v, DenseNodeID u) {
  auto &data = m_physics->Data();
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(v, u, out);
  if (IsZero(out, m_dimension))
    return;
  Vecaxpy((pow(m_distanceCalc.BetweenNodes(v, u), 2.0) / (10.0 * 10.0)), out,
          data[v.Raw()].disp, m_dimension);
}

template <GraphLike G>
void GRIPFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v, DenseNodeID u) {
  auto &data = m_physics->Data();
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(u, v, out);
  if (IsZero(out, m_dimension))
    return;
  Vecaxpy((0.05 * (10.0 * 10.0) / pow(m_distanceCalc.BetweenNodes(v, u), 2.0)),
          out, data[v.Raw()].disp, m_dimension);
}

template <GraphLike G>
void GRIPFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v, const double *uPos,
                                               double uMass) {
  auto &data = m_physics->Data();
  double out[m_dimension];
  Negate(m_distanceCalc.VecToPoint(v, uPos, out), m_dimension);
  if (IsZero(out, m_dimension))
    return;
  Vecaxpy((0.05 * (10.0 * 10.0) / pow(m_distanceCalc.ToPoint(v, uPos), 2.0)),
          out, data[v.Raw()].disp, m_dimension);
}
