#pragma once

#include "ds/quadtree.hpp"
#include <cassert>

namespace SpacialIndex {

template <typename T>
QuadTree<T>::QuadTree(uint32_t nodeCapacity, const AABB &aabb)
    : m_nodeCapacity(nodeCapacity), m_northEast(nullptr), m_northWest(nullptr),
      m_mass(0), m_southEast(nullptr), m_southWest(nullptr), m_bounds(aabb),
      m_com{0, 0} {}

template <typename T> QuadTree<T>::~QuadTree() {
  if (m_northWest)
    delete m_northWest;
  if (m_northEast)
    delete m_northEast;
  if (m_southWest)
    delete m_southWest;
  if (m_southEast)
    delete m_southEast;
}

template <typename T> void QuadTree<T>::Subdivide() {
  if (m_northWest)
    return;
  double quarterLength = m_bounds.halfLength / 2.0;
  double cx = m_bounds.center[0];
  double cy = m_bounds.center[0];
  m_northWest = new QuadTree<T>{
      m_nodeCapacity,
      AABB{{cx - quarterLength, cy + quarterLength}, quarterLength}};
  m_northEast = new QuadTree<T>{
      m_nodeCapacity,
      AABB{{cx + quarterLength, cy + quarterLength}, quarterLength}};
  m_southWest = new QuadTree<T>{
      m_nodeCapacity,
      AABB{{cx - quarterLength, cy - quarterLength}, quarterLength}};
  m_southEast = new QuadTree<T>{
      m_nodeCapacity,
      AABB{{cx + quarterLength, cy - quarterLength}, quarterLength}};
}

template <typename T>
bool QuadTree<T>::Insert(const T &data, Point p, double mass) {
  if (!m_bounds.contains(p))
    return false;

  double newMass = m_mass + mass;
  m_com[0] = (m_com[0] * m_mass + p[0] * mass) / newMass;
  m_com[1] = (m_com[1] * m_mass + p[1] * mass) / newMass;
  mass = newMass;

  if (m_points.size() < m_nodeCapacity && !m_northWest) {
    m_points.push_back(p);
    m_data.push_back(data);
    return true;
  }

  if (!m_northWest) {
    Subdivide();

    size_t s = m_data.size();
    for (size_t i = 0; i < s; i++) {
      Point pt = m_points[i];
      T d = m_data[i];

      auto child = QuadrantFor(pt);
      child->Insert(d, pt);
    }
    m_data.clear();
    m_points.clear();
  }

  auto child = QuadrantFor(p);
  if (child->Insert(data, p))
    return true;

  assert(false);
}

template <typename T>
QuadTreeQuadrant QuadTree<T>::QuadrantFor(const Point &p) {
  bool north = p[1] - m_bounds.center[1] > 0;
  bool east = p[0] - m_bounds.center[0] > 0;
  if (north)
    return east ? NE : NW;
  return east ? SE : SW;
}

template <typename T>
QuadTree<T> *QuadTree<T>::Quadrant(const QuadTreeQuadrant &quadrant) {
  switch (quadrant) {
  case NW:
    return m_northWest;
  case NE:
    return m_northEast;
  case SW:
    return m_southWest;
  case SE:
    return m_southEast;
  }
}

template <typename T>
std::vector<typename QuadTree<T>::QuadTreeNode>
QuadTree<T>::QueryRange(const AABB &range) const {
  std::vector<QuadTreeNode> found;

  if (!m_bounds.intersects(range))
    return found;

  if (!m_northWest) {
    size_t s = m_points.size();
    for (size_t i = 0; i < s; i++) {
      found.emplace_back(m_points[i], m_data[i]);
    }
    return found;
  }

  found.insert(m_northWest->QueryRange(range));
  found.insert(m_northEast->QueryRange(range));
  found.insert(m_southWest->QueryRange(range));
  found.insert(m_southEast->QueryRange(range));

  return found;
}

} // namespace SpacialIndex
