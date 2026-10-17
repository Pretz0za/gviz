#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "graph/types.hpp"
#include "layout/physics/force_model/linlog.hpp"
#include <cmath>
#include <cstdint>

template <GraphLike G>
LinLog<G>::LinLog(G &graph, Configuration *config)
    : m_config(RequireConfiguration(config)), m_distanceCalc(graph),
      m_positions(graph.NodeSpace().template GetPool<PositionComponent>()),
      m_physics(graph.NodeSpace().template GetPool<PhysicsComponent>()),
      m_degree(graph.NodeSpace().template GetPool<DegreeComponent>()) {
  DimensionResource *dim = graph.template GetResource<DimensionResource>();
  if (dim == nullptr) {
    throw MissingResourceException<DimensionResource>();
  }
  m_dimension = static_cast<uint8_t>(*dim);
}

template <GraphLike G> double LinLog<G>::VertexMass(DenseNodeID v) const {
  const DegreeComponent *deg = m_degree->Find(v.Raw());
  return 1.0 + static_cast<double>(deg->in + deg->out);
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
