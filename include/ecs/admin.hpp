#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "icomponent_pool.hpp"

template <typename T> class ComponentPool;

class Admin {
public:
  Admin();
  ~Admin() = default;

  EntityID CreateEntity();

  void DestroyEntity(EntityID id);

  template <typename T> ComponentPool<T> &GetPool();

  template <typename T> T &AddComponent(EntityID id);

  template <typename T> T *GetComponent(EntityID id);

  template <typename T> bool HasComponent(EntityID id);

private:
  EntityID m_nextID = 0;
  std::vector<EntityID> m_freeIDs;
  // TODO: can give component pools ids and make this an array
  std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};

// Iterator to a specific ComponentPool of the Admin
template <typename T> class ComponentItr {

  ComponentItr(const Admin &admin) : m_pool(admin.GetPool<T>()), m_idx(0) {}

  ComponentItr begin() { return m_pool->Data().begin(); }
  ComponentItr end() { return m_pool->Data().end(); };

  bool operator!=(const ComponentItr &other) { return m_idx != other.m_idx; };
  void operator++() { ++m_idx; };
  T *operator*() { return &m_pool->Data()[m_idx]; };

private:
  ComponentPool<T> *m_pool;
  uint32_t m_idx;
};

#include "admin.tpp"
#include "components.hpp"
