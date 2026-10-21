#pragma once

#include "layout/planarity/schnyder_wood.hpp"
#include <stdexcept>

template <GraphLike G>
SchnyderWood<G>::SchnyderWood(PlanarRotationSystem<G> &rotation, NodeID s0, NodeID s1)
    : m_g(rotation.Graph()) {
  G &g = m_g;
  auto D = [&](NodeID v) { return g.MapToDense(v).Raw(); };

  uint32_t n = g.Size();
  if (n < 3)
    throw std::runtime_error("SchnyderWood requires at least 3 vertices");

  for (auto &p : m_parent)
    p.assign(n, INVALID_NODE_ID);

  NodeID s2 = rotation.FaceThirdVertex(s0, s1);

  m_root[0] = s0;
  m_root[1] = s1;
  m_root[2] = s2;

  if (n == 3) {
    m_parent[0][D(s2)] = s0;
    m_parent[1][D(s2)] = s1;
    return;
  }

  std::vector<NodeID> bnext(n, INVALID_NODE_ID), bprev(n, INVALID_NODE_ID);
  std::vector<bool> done(n, false);
  std::vector<bool> onBoundary(n, false);

  bnext[D(s0)] = s1;
  bprev[D(s1)] = s0;
  done[D(s0)] = true;
  done[D(s1)] = true;
  onBoundary[D(s0)] = true;
  onBoundary[D(s1)] = true;

  auto inArc = [&](NodeID x, NodeID from, NodeID to) {
    NodeID cur = from;
    while (true) {
      if (cur == x)
        return true;
      if (cur == to)
        return false;
      cur = bnext[D(cur)];
      if (!cur.IsValid())
        return false;
    }
  };

  uint32_t remaining = n - 3;

  while (remaining > 0) {
    NodeID w = INVALID_NODE_ID;
    NodeID Lp = INVALID_NODE_ID, Rp = INVALID_NODE_ID;

    NodeID L = s0;
    while (bnext[D(L)].IsValid()) {
      NodeID R = bnext[D(L)];
      NodeID cand = rotation.FaceThirdVertex(R, L);

      if (cand != s2 && !done[D(cand)]) {
        NodeID candLp = L;
        while (bprev[D(candLp)].IsValid() && rotation.HasEdge(cand, bprev[D(candLp)]))
          candLp = bprev[D(candLp)];

        NodeID candRp = R;
        while (bnext[D(candRp)].IsValid() && rotation.HasEdge(cand, bnext[D(candRp)]))
          candRp = bnext[D(candRp)];

        bool strayNeighbor = false;
        for (const AdjEntry &adj : rotation.Neighbors(cand)) {
          if (!onBoundary[D(adj.other)])
            continue;
          if (!inArc(adj.other, candLp, candRp)) {
            strayNeighbor = true;
            break;
          }
        }

        bool unsafeHide = false;
        if (!strayNeighbor) {
          for (NodeID c = bnext[D(candLp)]; c != candRp && !unsafeHide; c = bnext[D(c)]) {
            for (const AdjEntry &adj : rotation.Neighbors(c)) {
              if (adj.other == cand || done[D(adj.other)])
                continue;
              unsafeHide = true;
              break;
            }
          }
        }

        if (!strayNeighbor && !unsafeHide) {
          w = cand;
          Lp = candLp;
          Rp = candRp;
          break;
        }
      }
      L = R;
    }

    if (!w.IsValid())
      throw std::runtime_error(
          "SchnyderWood: canonical-ordering scan found no candidate vertex");

    m_parent[0][D(w)] = Lp;
    m_parent[1][D(w)] = Rp;

    NodeID c = bnext[D(Lp)];
    while (c != Rp) {
      m_parent[2][D(c)] = w;
      NodeID nc = bnext[D(c)];
      bnext[D(c)] = INVALID_NODE_ID;
      bprev[D(c)] = INVALID_NODE_ID;
      onBoundary[D(c)] = false;
      c = nc;
    }

    bnext[D(Lp)] = w;
    bprev[D(w)] = Lp;
    bnext[D(w)] = Rp;
    bprev[D(Rp)] = w;

    done[D(w)] = true;
    onBoundary[D(w)] = true;
    remaining--;
  }

  NodeID cv = bnext[D(s0)];
  while (cv != s1 && cv.IsValid()) {
    m_parent[2][D(cv)] = s2;
    cv = bnext[D(cv)];
  }
  m_parent[0][D(s2)] = s0;
  m_parent[1][D(s2)] = s1;
}

template <GraphLike G> NodeID SchnyderWood<G>::Parent(size_t tree, NodeID v) const {
  return m_parent[tree][m_g.MapToDense(v).Raw()];
}
