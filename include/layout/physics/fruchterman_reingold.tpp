#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "graph/types.hpp"
#include "layout/physics/fruchterman_reingold.hpp"
#include <cmath>

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
  double dist = m_distanceCalc.BetweenNodes(v, u);

  Vecaxpy(dist / m_edgeLength, out, m_physics->Find(v.Raw())->disp,
          m_dimension);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v,
                                                  DenseNodeID u) {
  double out[m_dimension];
  // v - u (not u - v): repulsion pushes v away from u.
  m_distanceCalc.VecBetweenNodes(u, v, out);
  double dist = m_distanceCalc.BetweenNodes(v, u);

  Vecaxpy(m_edgeLength * m_edgeLength / (dist * dist), out,
          m_physics->Find(v.Raw())->disp, m_dimension);
}

template <GraphLike G>
void VanillaFruchtermanReingold<G>::RepulsiveTick(DenseNodeID v, const double *uPos,
                                                  double uMass) {
  double out[m_dimension];
  // v - u (not u - v): repulsion pushes v away from u.
  auto vPos = *m_positions->Find(v.Raw());
  Subtract(uPos, vPos.pos, out, m_dimension);
  double dist = L2Norm(out, m_dimension);

  Vecaxpy(m_edgeLength * m_edgeLength / (dist * dist), out,
          m_physics->Find(v.Raw())->disp, m_dimension);
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
