#pragma once

#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include <cstdint>
#include <utility>
#include <vector>

// Shared shape for any component that stores exactly one double per embedding
// dimension (e.g. various 'position' data). `Tag` is an empty type that
// distinguishes components with this same shape from one another.

// non-owning pointer to memory with doubles as per the dimension
template <typename Tag> struct DimensionalComponent : Component {
  double pos[1];
};

template <typename Tag> class DimensionalComponentSpan {
public:
  DimensionalComponentSpan(std::vector<double> &storage, uint8_t dim)
      : m_storage(storage), m_dim(dim) {}

  DimensionalComponent<Tag> &operator[](size_t idx) const {
    return *reinterpret_cast<DimensionalComponent<Tag> *>(
        &m_storage[idx * m_dim]);
  }

  size_t Size() const { return m_storage.size() / m_dim; }

  class iterator {
  public:
    iterator(double *ptr, uint8_t dim) : m_ptr(ptr), m_dim(dim) {}
    DimensionalComponent<Tag> &operator*() const {
      return *reinterpret_cast<DimensionalComponent<Tag> *>(m_ptr);
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

// type-erased component pool shared by every DimensionalComponent<Tag>,
// regardless of Tag. runtime defined stride by SetDimension.
template <typename Tag>
class DenseComponentPool<DimensionalComponent<Tag>>
    : public IDenseStorageListener {
public:
  DenseComponentPool(const IndexSpace &admin, uint8_t dimension)
      : m_dim(dimension), m_data(m_storage, dimension) {}
  ~DenseComponentPool() override = default;

  // must be called before any entities are added to this pool.
  void SetDimension(uint8_t dim) { m_dim = dim; }
  uint8_t Dimension() const { return m_dim; }

  void OnAdd() override {
    for (uint8_t i = 0; i < m_dim; i++)
      m_storage.push_back(0.0);
  }

  inline DimensionalComponent<Tag> *Find(EntityID id) {
    return const_cast<DimensionalComponent<Tag> *>(
        std::as_const(*this).Find(id));
  }

  inline const DimensionalComponent<Tag> *Find(EntityID id) const {
    if (id >= Size())
      return nullptr;
    return &m_data[id];
  }

  inline DimensionalComponentSpan<Tag> &Data() { return m_data; }
  inline const DimensionalComponentSpan<Tag> &Data() const { return m_data; }

  inline size_t Size() const { return m_data.Size(); }

private:
  std::vector<double> m_storage;
  uint8_t m_dim = 3;
  DimensionalComponentSpan<Tag> m_data;
};
