#pragma once

#include "ds/vector.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace SpacialIndex {

using Point = std::array<double, 2>;

enum QuadTreeQuadrant { NW, NE, SW, SE };

typedef struct AABB {
  Point center;
  double halfLength;

  inline bool contains(Point p) const {
    Subtract(p.data(), center.data(), p.data(), 2);
    return (fabs(p[0]) < halfLength && fabs(p[1]) < halfLength);
  }

  inline bool intersects(const AABB &other) const {
    // TODO:
    return true;
  };
} AABB;

template <typename T> class QuadTree {
public:
  struct QuadTreeNode {
    double mass;
    Point pos;
    T data;
  };

  QuadTree(uint32_t nodeCapacity, const AABB &aabb);
  ~QuadTree();

  bool Insert(const T &data, Point p, double mass = 1);
  std::vector<QuadTreeNode> QueryRange(const AABB &range) const;

  inline Point CenterOfMass() const {return m_com;}
  inline QuadTree<T> *Quadrant(const QuadTreeQuadrant &quadrant);

private:
  void Subdivide();

  inline QuadTreeQuadrant QuadrantFor(const Point &p);

  std::vector<Point> m_points;
  std::vector<T> m_data;
  QuadTree<T> *m_northWest;
  QuadTree<T> *m_northEast;
  QuadTree<T> *m_southWest;
  QuadTree<T> *m_southEast;
  uint32_t m_nodeCapacity = 1;
  AABB m_bounds;
  Point m_com;
  double m_mass;
};
}; // namespace SpacialIndex

#include "ds/quadtree.tpp"
