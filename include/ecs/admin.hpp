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

  ComponentItr(const Admin &admin) : pool(admin.GetPool<T>()), idx(0) {}

  ComponentItr begin();
  ComponentItr end();

  bool operator!=(const ComponentItr &other);
  void operator++();
  T *operator*();

private:
  ComponentPool<T> *pool;
  uint32_t idx;
};

// ComponentPool<T> must be complete before the template bodies below.
#include "components.hpp"
#include "admin.tpp"
