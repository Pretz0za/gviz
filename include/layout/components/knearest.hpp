#pragma once

#include "ecs/components.hpp"
#include "ecs/index_space.hpp"
#include "graph/search/types.hpp"
#include <cstdint>
#include <utility>
#include <vector>

// non-owning view over one entity's k-nearest-neighbor results
struct KNearestComponent : Component {
  uint32_t size;
  FoundNode nearest[1];
};

class KNearestSpan {
public:
  KNearestSpan(std::vector<uint32_t> &storage, uint32_t &k)
      : m_storage(storage), m_k(k) {}

  KNearestComponent &operator[](size_t idx) const {
    return *reinterpret_cast<KNearestComponent *>(&m_storage[idx * stride()]);
  }

  size_t size() const { return m_storage.size() / stride(); }

  class iterator {
  public:
    iterator(uint32_t *ptr, size_t stride) : m_ptr(ptr), m_stride(stride) {}
    KNearestComponent &operator*() const {
      return *reinterpret_cast<KNearestComponent *>(m_ptr);
    }
    iterator &operator++() {
      m_ptr += m_stride;
      return *this;
    }
    bool operator!=(const iterator &other) const {
      return m_ptr != other.m_ptr;
    }

  private:
    uint32_t *m_ptr;
    size_t m_stride;
  };

  iterator begin() const { return iterator(m_storage.data(), stride()); }
  iterator end() const {
    return iterator(m_storage.data() + m_storage.size(), stride());
  }

private:
  size_t stride() const { return 1 + 2 * static_cast<size_t>(m_k); }

  std::vector<uint32_t> &m_storage;
  uint32_t &m_k;
};

// type-erased component pool for k-nearest-neighbor results. runtime
// defined stride by SetK.
template <>
class DenseComponentPool<KNearestComponent> : public IDenseStorageListener {
public:
  DenseComponentPool(uint32_t k) : m_k(k), m_data(m_storage, m_k) {}
  ~DenseComponentPool() override = default;

  // must be called before any entities are added to this pool.
  void SetK(uint32_t k) { m_k = k; }
  uint32_t K() const { return m_k; }

  void OnAdd() override {
    for (uint32_t i = 0; i < 1 + 2 * m_k; i++)
      m_storage.push_back(0);
  }

  KNearestComponent *Find(EntityID id) {
    return const_cast<KNearestComponent *>(std::as_const(*this).Find(id));
  }

  const KNearestComponent *Find(EntityID id) const {
    if (id >= Size())
      return nullptr;
    return &m_data[id];
  }

  KNearestSpan &Data() { return m_data; }
  const KNearestSpan &Data() const { return m_data; }

  size_t Size() const { return m_data.size(); }

private:
  std::vector<uint32_t> m_storage;
  uint32_t m_k = 32;
  KNearestSpan m_data;
};
