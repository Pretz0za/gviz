#pragma once

#include <cstdint>
#include <vector>

#include "entity.hpp"
#include "index_space.hpp"

template <typename T> class DenseComponentPool : public IDenseStorageListener {
public:
  explicit DenseComponentPool(IndexSpace &space);
  ~DenseComponentPool() override = default;

  void OnAdd() override;
  void OnRemove(uint32_t index) override;

  T *Find(EntityID id);
  const T *Find(EntityID id) const;

  std::vector<T> &Data();
  const std::vector<T> &Data() const;

  EntityID Owner(uint32_t local) const;

  size_t Size() const;

  uint32_t CompactIndex(EntityID id) const;

private:
  std::vector<T> m_data;
  IndexSpace &m_space;
};

struct Component {};

#include "components.tpp"
