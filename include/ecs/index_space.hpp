#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "entity.hpp"

template <typename T> class DenseComponentPool;

class IDenseStorageListener {
public:
  virtual ~IDenseStorageListener() = default;
  virtual void OnAdd() = 0;
};

class IndexSpace {
public:
  IndexSpace() = default;
  IndexSpace(const IndexSpace &) = delete;
  IndexSpace &operator=(const IndexSpace &) = delete;
  IndexSpace(IndexSpace &&) = default;
  IndexSpace &operator=(IndexSpace &&) = default;

  EntityID Create();
  EntityID Create(EntityID sparseID);

  inline EntityID MapToDense(EntityID sparseID) const {
    return m_mapToDense.at(sparseID);
  }
  inline EntityID MapToSparse(EntityID denseID) const {
    return m_mapToSparse[denseID];
  }

  size_t Size() const;

  template <typename T, typename... Args>
  DenseComponentPool<T> *SetPool(Args &&...args);
  template <typename T> DenseComponentPool<T> *GetPool() const;

private:
  uint32_t m_size = 0;
  std::unordered_map<EntityID, EntityID> m_mapToDense;
  std::vector<EntityID> m_mapToSparse;
  std::unordered_map<std::type_index, std::unique_ptr<IDenseStorageListener>>
      m_pools;
};

#include "index_space.tpp"
