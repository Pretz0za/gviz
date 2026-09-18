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

  inline T *Find(EntityID id);
  inline const T *Find(EntityID id) const;

  inline std::vector<T> &Data();
  inline const std::vector<T> &Data() const;

  uint32_t Size() const;

private:
  std::vector<T> m_data;
};

struct Component {};

#include "components.tpp"
