#pragma once

#include "admin.hpp"
#include <memory>
#include <typeindex>

template <typename T> ComponentPool<T> &Admin::GetPool() {
  auto key = std::type_index(typeid(T));
  auto it = m_pools.find(key);
  if (it == m_pools.end()) {
    auto pool = std::make_unique<ComponentPool<T>>(*this);
    auto *raw = pool.get();
    m_pools.emplace(key, std::move(pool));
    return *raw;
  }
  return *static_cast<ComponentPool<T> *>(it->second.get());
}

template <typename T> T &Admin::AddComponent(EntityID id) {
  T &out = GetPool<T>().Add(id);
  out.m_owner = id;
  return out;
}

// TODO: these could be removed maybe? GetPool can be made the only thing one
// should use to access components
template <typename T> T *Admin::GetComponent(EntityID id) {
  return GetPool<T>().Find(id);
}

template <typename T> bool Admin::HasComponent(EntityID id) {
  return GetPool<T>().Find(id) != nullptr;
}
