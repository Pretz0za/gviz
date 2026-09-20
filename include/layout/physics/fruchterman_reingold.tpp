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

  // accumulate into disp (like GRIPFruchtermanReingold/GravityForceSystem);
  // ForceDirectedLayoutAlgorithm::Tick applies the total disp to position
  // once per tick, after clamping it.
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
