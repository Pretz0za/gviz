#pragma once

#include "concept/graphLike.hpp"
#include "ds/vector.hpp"
#include "memory/arena.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>
namespace SpacialIndex {


enum QuadTreeQuadrant { NW, NE, SW, SE, COUNT };

using Point = std::array<double, 2>;

typedef struct AABB {
  Point center;
  double halfLength;

  inline bool contains(Point p) const {
    Subtract(p.data(), center.data(), p.data(), 2);
    return (fabs(p[0]) <= halfLength && fabs(p[1]) <= halfLength);
  }

  inline bool intersects(const AABB &other) const {
    return fabs(center[0] - other.center[0]) <= (halfLength + halfLength) &&
           fabs(center[1] - center[1]) <= (halfLength + halfLength);
  };
} AABB;

template <typename T, uint32_t S> class QuadTree {
public:
  struct QuadTreeNode {
    double mass;
    Point pos;
    T data;
  };

  QuadTree();
  QuadTree(const AABB &aabb);
  ~QuadTree() {
    for (uint32_t i = 0; i < m_pointCount; i++) {
      pointPtr(i)->~Point();
      dataPtr(i)->~T();
    }
  }

  bool Insert(const T &data, Point p, double mass = 1);
  std::vector<QuadTreeNode> QueryRange(const AABB &range) const;

  inline double Mass() const { return m_mass; }
  inline Point CenterOfMass() const { return m_com; }
  inline QuadTree<T, S> *Quadrant(const QuadTreeQuadrant &quadrant);
  inline bool IsRoot() const { return m_ownedArena.get() != nullptr; }
  inline bool IsLeaf() const { return !m_northWest; }
  inline bool IsEmpty() const { return m_pointCount == 0; }
  inline Point PointAt(uint32_t i) const { return *pointPtr(i); }
  inline T DataAt(uint32_t i) const { return *dataPtr(i); }
  inline double HalfLength() const { return m_bounds.halfLength; }
  inline const Point Center() const { return m_bounds.center; }

  void Reset(const AABB &aabb);

private:
  // Lets Memory::Arena::Allocate<QuadTree<T>> reach the constructor below
  // when it places child nodes directly into arena-owned storage.
  friend class Memory::Arena;

  // Used to construct children that share the parent's arena instead of
  // each owning their own.
  QuadTree(const AABB &aabb, Memory::Arena *arena);

  void Subdivide();

  inline QuadTreeQuadrant QuadrantFor(const Point &p);

  Point *pointPtr(size_t i) {
    return std::launder(
        reinterpret_cast<Point *>(m_pointsStorage + i * sizeof(Point)));
  }
  const Point *pointPtr(size_t i) const {
    return std::launder(
        reinterpret_cast<const Point *>(m_pointsStorage + i * sizeof(Point)));
  }
  T *dataPtr(size_t i) {
    return std::launder(reinterpret_cast<T *>(m_dataStorage + i * sizeof(T)));
  }
  const T *dataPtr(size_t i) const {
    return std::launder(
        reinterpret_cast<const T *>(m_dataStorage + i * sizeof(T)));
  }

  // Only set (non-null) on the root node; every node allocates through
  // m_arena, which points at the root's arena for the whole tree.
  unique_ptr<Memory::Arena> m_ownedArena;
  Memory::Arena *m_arena;

  QuadTree<T, S> *m_northWest;
  QuadTree<T, S> *m_southWest;
  QuadTree<T, S> *m_northEast;
  QuadTree<T, S> *m_southEast;

  Point m_com;
  double m_mass;

  AABB m_bounds;
  alignas(Point) std::byte m_pointsStorage[S * sizeof(Point)];
  alignas(T) std::byte m_dataStorage[S * sizeof(T)];
  uint32_t m_pointCount = 0;
};

namespace Helpers {
template <GraphLike G> AABB GetBoundingBox(G *graph);
};

}; // namespace SpacialIndex

#include "ds/quadtree.tpp"
