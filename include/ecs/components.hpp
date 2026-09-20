#pragma once

#include <cstdint>
#include <vector>

#include "entity.hpp"
#include "index_space.hpp"

template <typename T> class DenseComponentPool : public IDenseStorageListener {
public:
  DenseComponentPool(const IndexSpace &admin);
  ~DenseComponentPool() override = default;

  void OnAdd() override;
  void Set(EntityID id, T data);

  inline T *Find(EntityID id);
  inline const T *Find(EntityID id) const;

  inline std::vector<T> &Data();
  inline const std::vector<T> &Data() const;

  uint32_t Size() const;

  template <typename U> DenseComponentPool<U> Sibling() const;

private:
  std::vector<T> m_data;
  const IndexSpace &m_admin;
};

struct Component {};

#include "components.tpp"
