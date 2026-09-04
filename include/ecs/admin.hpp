#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "entity.hpp"
#include "index_space.hpp"
#include "resource.hpp"

template <typename T> class DenseComponentPool;

class Admin {
public:
  Admin();
  Admin(const Admin &) = delete;
  Admin &operator=(const Admin &) = delete;
  Admin(Admin &&) = default;
  Admin &operator=(Admin &&) = default;
  ~Admin() = default;

  EntityID CreateEntity();

  void DestroyEntity(EntityID id);

  IndexSpace &CreateSpace();

  template <typename T> DenseComponentPool<T> &GetPool(IndexSpace &space);
  template <typename T> const DenseComponentPool<T> *TryGetPool() const;

  template <typename T> bool HasComponent(EntityID id) const;

  template <typename T, typename... Args> T &SetResource(Args &&...args);

  template <typename T> T *GetResource();

  template <typename T> bool HasResource() const;

private:
  EntityID m_nextID = 0;
  std::vector<EntityID> m_freeIDs;
  std::deque<IndexSpace> m_spaces;
  // TODO: can i make this not be a map?
  std::unordered_map<std::type_index, std::unique_ptr<IDenseStorageListener>>
      m_pools;
  std::unordered_map<std::type_index, std::unique_ptr<IResourceHolder>>
      m_resources;
};

// Iterator to a specific ComponentPool of the Admin
template <typename T> class ComponentItr {
public:
  ComponentItr(const Admin &admin) : m_pool(admin.GetPool<T>()), m_idx(0) {}

  ComponentItr begin() { return m_pool->Data().begin(); }
  ComponentItr end() { return m_pool->Data().end(); };

  bool operator!=(const ComponentItr &other) { return m_idx != other.m_idx; };
  void operator++() { ++m_idx; };
  T *operator*() { return &m_pool->Data()[m_idx]; };

private:
  DenseComponentPool<T> *m_pool;
  uint32_t m_idx;
};

#include "admin.tpp"
#include "components.hpp"
