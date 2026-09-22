#pragma once

#include "ds/quadtree.hpp"
#include "ds/vector.hpp"
#include "ecs/components.hpp"
#include "layout/components/position.hpp"
#include <algorithm>
#include <cassert>
#include <limits>

namespace SpacialIndex {

template <typename T, uint32_t S>
QuadTree<T, S>::QuadTree(const AABB &aabb) : QuadTree(aabb, nullptr) {}

template <typename T, uint32_t S>
QuadTree<T, S>::QuadTree()
    : QuadTree({{0, 0}, std::numeric_limits<double>::infinity()}, nullptr) {}

template <typename T, uint32_t S>
QuadTree<T, S>::QuadTree(const AABB &aabb, Memory::Arena *arena)
    : m_ownedArena(arena ? nullptr : std::make_unique<Memory::Arena>(8192)),
      m_arena(arena ? arena : m_ownedArena.get()), m_northWest(nullptr),
      m_northEast(nullptr), m_southWest(nullptr), m_southEast(nullptr),
      m_bounds(aabb), m_com{0, 0}, m_mass(0) {}

template <typename T, uint32_t S> void QuadTree<T, S>::Subdivide() {
  if (m_northWest)
    return;
  double quarterLength = m_bounds.halfLength / 2.0;
  double cx = m_bounds.center[0];
  double cy = m_bounds.center[1];
  m_northWest = m_arena->Allocate<QuadTree<T, S>>(

      AABB{{cx - quarterLength, cy + quarterLength}, quarterLength}, m_arena);
  m_northEast = m_arena->Allocate<QuadTree<T, S>>(

      AABB{{cx + quarterLength, cy + quarterLength}, quarterLength}, m_arena);
  m_southWest = m_arena->Allocate<QuadTree<T, S>>(

      AABB{{cx - quarterLength, cy - quarterLength}, quarterLength}, m_arena);
  m_southEast = m_arena->Allocate<QuadTree<T, S>>(

      AABB{{cx + quarterLength, cy - quarterLength}, quarterLength}, m_arena);
}

template <typename T, uint32_t S>
bool QuadTree<T, S>::Insert(const T &data, Point p, double mass) {
  if (!m_bounds.contains(p))
    return false;

  double newMass = m_mass + mass;
  m_com[0] = (m_com[0] * m_mass + p[0] * mass) / newMass;
  m_com[1] = (m_com[1] * m_mass + p[1] * mass) / newMass;
  m_mass = newMass;

  if (m_pointCount < S && !m_northWest) {
    ::new (pointPtr(m_pointCount)) Point(p);
    ::new (dataPtr(m_pointCount)) T(data);
    m_pointCount++;
    return true;
  }

  if (!m_northWest) {
    Subdivide();

    size_t s = m_pointCount;
    for (size_t i = 0; i < s; i++) {
      Point pt = *pointPtr(i);
      T d = *dataPtr(i);
      pointPtr(i)->~Point();
      dataPtr(i)->~T();

      auto child = Quadrant(QuadrantFor(pt));
      assert(child->Insert(d, pt));
    }
    m_pointCount = 0;
  }

  auto child = Quadrant(QuadrantFor(p));
  if (child->Insert(data, p))
    return true;

  assert(false);
}

template <typename T, uint32_t S> void QuadTree<T, S>::Reset(const AABB &aabb) {
  if (IsRoot()) {
    m_ownedArena->Reset();
    m_bounds = aabb;
  }
}

template <typename T, uint32_t S>
QuadTreeQuadrant QuadTree<T, S>::QuadrantFor(const Point &p) {
  bool north = p[1] - m_bounds.center[1] > 0;
  bool east = p[0] - m_bounds.center[0] > 0;
  if (north)
    return east ? NE : NW;
  return east ? SE : SW;
}

template <typename T, uint32_t S>
QuadTree<T, S> *QuadTree<T, S>::Quadrant(const QuadTreeQuadrant &quadrant) {
  switch (quadrant) {
  case NW:
    return m_northWest;
  case NE:
    return m_northEast;
  case SW:
    return m_southWest;
  case SE:
    return m_southEast;
  default:
    return nullptr;
  }
}

template <typename T, uint32_t S>
std::vector<typename QuadTree<T, S>::QuadTreeNode>
QuadTree<T, S>::QueryRange(const AABB &range) const {
  std::vector<QuadTreeNode> found;

  if (!m_bounds.intersects(range))
    return found;

  if (!m_northWest) {
    size_t s = m_pointCount;
    for (size_t i = 0; i < s; i++) {
      found.emplace_back(*pointPtr(i), *dataPtr(i));
    }
    return found;
  }

  found.insert(m_northWest->QueryRange(range));
  found.insert(m_northEast->QueryRange(range));
  found.insert(m_southWest->QueryRange(range));
  found.insert(m_southEast->QueryRange(range));

  return found;
}

namespace Helpers {
template <GraphLike G> AABB GetBoundingBox(G *graph) {
  DenseComponentPool<PositionComponent> *pool =
      graph->NodeSpace().template GetPool<PositionComponent>();
  if (!pool) {
    return {{0, 0}, std::numeric_limits<double>::infinity()};
  }
  auto &positions = pool->Data();

  double minX = std::numeric_limits<double>::infinity(),
         maxX = -std::numeric_limits<double>::infinity();
  double minY = std::numeric_limits<double>::infinity(),
         maxY = -std::numeric_limits<double>::infinity();
  for (uint32_t i = 0; i < positions.Size(); i++) {
    minX = std::min(minX, positions[i].pos[0]);
    maxX = std::max(maxX, positions[i].pos[0]);
    minY = std::min(minY, positions[i].pos[1]);
    maxY = std::max(maxY, positions[i].pos[1]);
  }

  double span = std::max(maxX - minX, maxY - minY);
  double halfSize = span > EPSILON ? span / 2.0 : 1.0;
  double cx = (minX + maxX) / 2.0;
  double cy = (minY + maxY) / 2.0;

  return {{cx, cy}, halfSize};
}
}; // namespace helpers

} // namespace SpacialIndex
