#pragma once

#include <cstdint>
#include <vector>

#include "entity.hpp"
#include "index_space.hpp"

template <typename T> class DenseComponentPool : public IDenseStorageListener {
public:
  DenseComponentPool() = default;
  ~DenseComponentPool() override = default;

  void OnAdd() override;
  void Set(EntityID id, T data);

  T *Find(EntityID id);
  const T *Find(EntityID id) const;

  std::vector<T> &Data();
  const std::vector<T> &Data() const;

  size_t Size() const;

private:
  std::vector<T> m_data;
};

struct Component {};

#include "components.tpp"
