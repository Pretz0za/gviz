#pragma once

#include "layout/planarity/triangulation_system.hpp"
#include <cstddef>
#include <stdexcept>
#include <vector>

template <GraphLike G>
TriangulationSystem<G>::TriangulationSystem(PlanarRotationSystem<G> &rotation)
    : m_rotation(&rotation) {}

template <GraphLike G> void TriangulationSystem<G>::Compute() {
  std::vector<std::vector<NodeID>> faces = m_rotation->EnumerateFaces();

  for (size_t i = 0; i < faces.size(); i++) {
    while (faces[i].size() > 3) {
      size_t size = faces[i].size();
      bool split = false;

      for (size_t x = 0; x < size; x++) {
        size_t prevIdx = (x == 0 ? size : x) - 1;
        size_t nextIdx = (x + 1) % size;
        NodeID a = faces[i][prevIdx];
        NodeID xVertex = faces[i][x];
        NodeID b = faces[i][nextIdx];

        if (a == b || m_rotation->HasEdge(a, b))
          continue;

        size_t posInA = m_rotation->IndexOf(a, xVertex);
        m_rotation->InsertNeighborAt(a, posInA + 1, b);

        size_t posInB = m_rotation->IndexOf(b, xVertex);
        m_rotation->InsertNeighborAt(b, posInB, a);

        faces.push_back({a, xVertex, b});
        faces[i].erase(faces[i].begin() + static_cast<ptrdiff_t>(x));
        split = true;
        break;
      }

      if (!split)
        throw std::runtime_error("TriangulationSystem: no valid chord found in face");
    }
  }
}
