#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "graph/types.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"
#include <cmath>
#include <cstdint>

template <GraphLike G>
VanillaFruchtermanReingold<G>::VanillaFruchtermanReingold(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)), m_dimension(dg.dimension),
      m_distanceCalc(dg),
      m_positions(dg.nodeSpace.template GetPool<PositionComponent>()),
      m_physics(dg.nodeSpace.template GetPool<PhysicsComponent>()) {}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::AttractiveTick(DenseNodeID v,
                                                   DenseNodeID u) {
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(v, u, out);
  double gap = m_distanceCalc.BetweenNodes(v, u);
  Vecaxpy(std::fabs(gap) / m_config->edgeLength, out, m_physics->Find(v.Raw())->disp,
          m_dimension);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v,
                                                  DenseNodeID u) {
  double out[m_dimension];
  // v - u (not u - v): repulsion pushes v away from u.
  m_distanceCalc.VecBetweenNodes(u, v, out);
  double dist = m_distanceCalc.BetweenNodes(v, u);

  // See Configuration::minDistFraction: floor the magnitude's distance so a
  // near-zero (or negative, once overlapping) gap can't blow this up.
  double safeDist = std::fmax(std::fabs(dist), m_config->edgeLength * m_config->minDistFraction);
  Vecaxpy(m_config->edgeLength * m_config->edgeLength / (safeDist * safeDist), out,
          m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v, const double *uPos,
                                                  double uMass) {
  double out[m_dimension];
  const PositionComponent *vPos = m_positions->Find(v.Raw());
  Subtract(vPos->pos, uPos, out, m_dimension);
  double dist = L2Norm(out, m_dimension);

  if (dist < 1e-9) {
    return;
  }

  double safeDist = std::fmax(dist, m_config->edgeLength * m_config->minDistFraction);
  double mag = (m_config->edgeLength * m_config->edgeLength) / safeDist;
  Vecaxpy(mag / dist, out, m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
GRIPFruchtermanReingold<G>::GRIPFruchtermanReingold(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)),
      m_physics(dg.nodeSpace.template GetPool<PhysicsComponent>()),
      m_distanceCalc(dg), m_dimension(dg.dimension) {}

template <GraphLike G>
void GRIPFruchtermanReingold<G>::AttractiveTick(DenseNodeID v, DenseNodeID u) {
  auto &data = m_physics->Data();
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(v, u, out);
  if (IsZero(out, m_dimension))
    return;
  double spacing = m_config->targetSpacing;
  Vecaxpy((pow(m_distanceCalc.BetweenNodes(v, u), 2.0) / (spacing * spacing)), out,
          data[v.Raw()].disp, m_dimension);
}

template <GraphLike G>
void GRIPFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v, DenseNodeID u) {
  auto &data = m_physics->Data();
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(u, v, out);
  if (IsZero(out, m_dimension))
    return;
  double spacing = m_config->targetSpacing;
  Vecaxpy((m_config->repulsionStrength * (spacing * spacing) / pow(m_distanceCalc.BetweenNodes(v, u), 2.0)),
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
  double spacing = m_config->targetSpacing;
  Vecaxpy((m_config->repulsionStrength * (spacing * spacing) / pow(m_distanceCalc.ToPoint(v, uPos), 2.0)),
          out, data[v.Raw()].disp, m_dimension);
}
