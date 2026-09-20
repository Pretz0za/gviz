#pragma once

#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/degree.hpp"
#include <utility>

#define DEFAULT_RADIUS 8.0
#define MAX_RADIUS 32.0
#define HALF_SATURATION_DEGREE 6.0

typedef struct RadiusComponent : Component {
  double radius = DEFAULT_RADIUS;
} RadiusComponent;

enum RadiusFunction {
  MICHEALIS_MENTEN,
  MANUAL,
};

class RadiusSpan;

template <>
class DenseComponentPool<RadiusComponent> : public IDenseStorageListener {
public:
  DenseComponentPool(const IndexSpace &admin, RadiusFunction function = MANUAL)
      : m_degrees(admin.GetPool<DegreeComponent>()),
        m_inAdj(admin.GetPool<InAdjacencyComponent>()),
        m_outAdj(admin.GetPool<OutAdjacencyComponent>()), m_data(),
        m_function(function) {}
  ~DenseComponentPool() override = default;

  void ChangeRadiusFunction(RadiusFunction target) {
    if (target == MANUAL) {
      if (m_function == MANUAL)
        return;
      size_t targetSize = m_degrees ? m_degrees->Size() : m_outAdj->Size();
      m_data.resize(targetSize);
    }
    m_function = target;
  }

  void OnAdd() override {
    if (m_function == MANUAL) {
      m_data.emplace_back();
    }
  }

  RadiusComponent *Find(EntityID id) {
    return const_cast<RadiusComponent *>(std::as_const(*this).Find(id));
  }

  const RadiusComponent *Find(EntityID id) const {
    if (m_function == MANUAL) {
      return &m_data[id];
    }

    if (m_degrees) {
      auto *degree = m_degrees->Find(id);
      if (!degree)
        return nullptr;
      return degreeToRadius(*degree);
    } else if (m_outAdj && m_inAdj) {
      DegreeComponent degree{
          {},
          static_cast<uint32_t>(m_inAdj->Find(id)->in.size()),
          static_cast<uint32_t>(m_outAdj->Find(id)->out.size())};
      return degreeToRadius(degree);
    }
    return nullptr;
  }

  inline RadiusSpan Data();

  size_t Size() const {
    if (m_function == MANUAL)
      return m_data.size();
    return m_degrees ? m_degrees->Size() : m_inAdj->Size();
  }

private:
  RadiusComponent *degreeToRadius(const DegreeComponent &degree) const {
    switch (m_function) {
    case MICHEALIS_MENTEN: {
      double total =
          static_cast<double>(degree.in) + static_cast<double>(degree.out);
      double t = total / (total + HALF_SATURATION_DEGREE);
      m_out.radius = DEFAULT_RADIUS + (MAX_RADIUS - DEFAULT_RADIUS) * t;
      return &m_out;
    }
    default: {
      return nullptr;
    }
    }
  }

  mutable RadiusComponent m_out;
  DenseComponentPool<DegreeComponent> *m_degrees;
  DenseComponentPool<InAdjacencyComponent> *m_inAdj;
  DenseComponentPool<OutAdjacencyComponent> *m_outAdj;
  std::vector<RadiusComponent> m_data;
  RadiusFunction m_function;
};

class RadiusSpan {
public:
  explicit RadiusSpan(DenseComponentPool<RadiusComponent> &pool)
      : m_pool(pool) {}

  RadiusComponent &operator[](size_t idx) const {
    return *m_pool.Find(static_cast<EntityID>(idx));
  }

  size_t size() const { return m_pool.Size(); }

  class iterator {
  public:
    iterator(DenseComponentPool<RadiusComponent> *pool, size_t idx)
        : m_pool(pool), m_idx(idx) {}
    RadiusComponent &operator*() const {
      return *m_pool->Find(static_cast<EntityID>(m_idx));
    }
    iterator &operator++() {
      ++m_idx;
      return *this;
    }
    bool operator!=(const iterator &other) const {
      return m_idx != other.m_idx;
    }

  private:
    DenseComponentPool<RadiusComponent> *m_pool;
    size_t m_idx;
  };

  iterator begin() const { return iterator(&m_pool, 0); }
  iterator end() const { return iterator(&m_pool, size()); }

private:
  DenseComponentPool<RadiusComponent> &m_pool;
};

inline RadiusSpan DenseComponentPool<RadiusComponent>::Data() {
  return RadiusSpan(*this);
}
