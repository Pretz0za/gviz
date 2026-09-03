#pragma once

#include "admin.hpp"
#include <memory>
#include <typeindex>

template <typename T> ComponentPool<T> &Admin::GetPool() {
  auto key = std::type_index(typeid(T));
  auto it = m_pools.find(key);
  if (it == m_pools.end()) {
    auto pool = std::make_unique<ComponentPool<T>>();
    auto *raw = pool.get();
    m_pools.emplace(key, std::move(pool));
    return *raw;
  }
  return *static_cast<ComponentPool<T> *>(it->second.get());
}

template <typename T> const ComponentPool<T> *Admin::TryGetPool() const {
  auto key = std::type_index(typeid(T));
  auto it = m_pools.find(key);
  if (it == m_pools.end())
    return nullptr;
  return static_cast<const ComponentPool<T> *>(it->second.get());
}

template <typename T> T &Admin::AddComponent(EntityID id) {
  T &out = GetPool<T>().Add(id);
  return out;
}

// TODO: these could be removed maybe? GetPool can be made the only thing one
// should use to access components
template <typename T> T *Admin::GetComponent(EntityID id) {
  return GetPool<T>().Find(id);
}

template <typename T> bool Admin::HasComponent(EntityID id) const {
  auto *pool = TryGetPool<T>();
  return pool != nullptr && pool->Find(id) != nullptr;
}

template <typename T, typename... Args> T &Admin::SetResource(Args &&...args) {
  auto key = std::type_index(typeid(T));
  auto holder = std::make_unique<ResourceHolder<T>>(std::forward<Args>(args)...);
  auto *raw = holder.get();
  m_resources[key] = std::move(holder);
  return raw->m_value;
}

template <typename T> T *Admin::GetResource() {
  auto key = std::type_index(typeid(T));
  auto it = m_resources.find(key);
  if (it == m_resources.end())
    return nullptr;
  return &static_cast<ResourceHolder<T> *>(it->second.get())->m_value;
}

template <typename T> bool Admin::HasResource() const {
  auto key = std::type_index(typeid(T));
  return m_resources.find(key) != m_resources.end();
}
