#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "graph/types.hpp"
#include "layout/physics/force_model/linlog.hpp"
#include <cmath>
#include <cstdint>

template <GraphLike G>
LinLog<G>::LinLog(DecoratedGraph<G> &dg, Configuration *config)
    : m_config(RequireConfiguration(config)), m_dimension(dg.dimension),
      m_distanceCalc(dg), m_graph(dg.graph),
      m_positions(dg.nodeSpace.template GetPool<PositionComponent>()),
      m_physics(dg.nodeSpace.template GetPool<PhysicsComponent>()) {}

template <GraphLike G> double LinLog<G>::VertexMass(DenseNodeID v) const {
  return 1.0 + static_cast<double>(m_graph->Degree(m_graph->MapToSparse(v)));
}

template <GraphLike G>
void LinLog<G>::AttractiveTick(DenseNodeID v, DenseNodeID u) {
  double out[m_dimension];
  // out = gap * unit(u - v).
  m_distanceCalc.VecBetweenNodes(v, u, out);
  double gap = m_distanceCalc.BetweenNodes(v, u);
  double absGap = std::fabs(gap);
  if (absGap < 1e-9)
    return; // coincident: no direction to pull along

  // force_vec = (unit in u's direction) * log1p(absGap)
  double scale = std::log1p(absGap) / gap;
  Vecaxpy(scale, out, m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
void LinLog<G>::RepulsiveTick(DenseNodeID v, DenseNodeID u) {
  double out[m_dimension];
  m_distanceCalc.VecBetweenNodes(u, v, out);
  double gap = m_distanceCalc.BetweenNodes(v, u);
  double absGap = std::fabs(gap);

  double safeDist =
      std::fmax(absGap, m_config->edgeLength * m_config->minDistFraction);
  double mag = (VertexMass(v) * VertexMass(u)) / safeDist;

  if (absGap < 1e-9) {
    return;
  }

  // out = gap * unit(v - u); mag/gap recovers unit(v - u)'s sign (dividing
  // by the signed gap) and rescales it to mag.
  Vecaxpy(mag / gap, out, m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
void LinLog<G>::RepulsiveTick(DenseNodeID v, const double *uPos, double uMass) {
  double out[m_dimension];
  const PositionComponent *vPos = m_positions->Find(v.Raw());
  Subtract(vPos->pos, uPos, out, m_dimension);
  double dist = L2Norm(out, m_dimension);

  if (dist < 1e-9) {
    return;
  }

  double safeDist =
      std::fmax(dist, m_config->edgeLength * m_config->minDistFraction);
  double mag = (VertexMass(v) * uMass) / safeDist;
  Vecaxpy(mag / dist, out, m_physics->Find(v.Raw())->disp, m_dimension);
}
