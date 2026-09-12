#pragma once

#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include <cstdint>
#include <utility>
#include <vector>

// non-owning pointer to memory with doubles as per the dimension
struct PositionComponent : Component {
  double pos[1];
};

class PositionSpan {
public:
  PositionSpan(std::vector<double> &storage, uint8_t dim)
      : m_storage(storage), m_dim(dim) {
	  }

  PositionComponent &operator[](size_t idx) const {
    return *reinterpret_cast<PositionComponent *>(&m_storage[idx * m_dim]);
  }

  size_t size() const {
    return m_storage.size() / m_dim;
  }

  class iterator {
  public:
    iterator(double *ptr, uint8_t dim) : m_ptr(ptr), m_dim(dim) {}
    PositionComponent &operator*() const {
      return *reinterpret_cast<PositionComponent *>(m_ptr);
    }
    iterator &operator++() {
      m_ptr += m_dim;
      return *this;
    }
    bool operator!=(const iterator &other) const {
      return m_ptr != other.m_ptr;
    }

  private:
    double *m_ptr;
    uint8_t m_dim;
  };

  iterator begin() const { return iterator(m_storage.data(), m_dim); }
  iterator end() const {
    return iterator(m_storage.data() + m_storage.size(), m_dim);
  }

private:
  std::vector<double> &m_storage;
  uint8_t m_dim;
};

// type-erased component pool for position. runtime defined stride by
// SetDimension
template <>
class DenseComponentPool<PositionComponent> : public IDenseStorageListener {
public:
  DenseComponentPool(uint8_t dimension)
      : m_dim(dimension), m_data(m_storage, dimension) {
	  }
  ~DenseComponentPool() override = default;

  // must be called before any entities are added to this pool.
  void SetDimension(uint8_t dim) { m_dim = dim; }
  uint8_t Dimension() const { return m_dim; }

  void OnAdd() override {
    for (uint8_t i = 0; i < m_dim; i++)
      m_storage.push_back(0.0);
  }

  PositionComponent *Find(EntityID id) {
    return const_cast<PositionComponent *>(std::as_const(*this).Find(id));
  }

  const PositionComponent *Find(EntityID id) const {
    if (id >= Size())
      return nullptr;
    return &m_data[id];
  }

  PositionSpan &Data() { return m_data; }
  const PositionSpan &Data() const { return m_data; }

  size_t Size() const {
    return m_data.size();
  }

private:
  std::vector<double> m_storage;
  uint8_t m_dim = 3;
  PositionSpan m_data;
};
