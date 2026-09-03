#pragma once

#include "ecs/components.hpp"
#include "ecs/icomponent_pool.hpp"
#include <cstdint>
#include <limits>
#include <utility>

template <typename T> ComponentPool<T>::ComponentPool() : m_local(), m_map() {}

template <typename T> T &ComponentPool<T>::Add(EntityID id) {
  size_t idx = m_local.size();
  m_map[id] = static_cast<uint32_t>(idx);
  m_owners.push_back(id);
  m_local.emplace_back();
  return m_local.back();
}

template <typename T> void ComponentPool<T>::Remove(EntityID id) {
  auto it = m_map.find(id);
  if (it == m_map.end())
    return;

  uint32_t idx = it->second;
  uint32_t last = m_local.size() - 1;

  m_map[m_owners[last]] = idx;
  std::swap(m_owners[idx], m_owners[last]);
  std::swap(m_local[idx], m_local[last]);
  m_local.pop_back();
  m_owners.pop_back();
  m_map.erase(id);
}

template <typename T> const T *ComponentPool<T>::Find(EntityID id) const {
  auto it = m_map.find(id);
  if (it == m_map.end())
    return nullptr;
  return &m_local[it->second];
}

template <typename T> T *ComponentPool<T>::Find(EntityID id) {
  return const_cast<T *>(std::as_const(*this).Find(id));
}

template <typename T> const std::vector<T> &ComponentPool<T>::Data() const {
  return m_local;
}

template <typename T> EntityID ComponentPool<T>::Owner(uint32_t local) const {
  return m_owners[local];
}

template <typename T> std::vector<T> &ComponentPool<T>::Data() {
  return const_cast<std::vector<T> &>(std::as_const(*this).Data());
}

template <typename T> size_t ComponentPool<T>::Size() const {
  return m_local.size();
}

template <typename T> uint32_t ComponentPool<T>::ToLocal(EntityID id) const {
  auto it = m_map.find(id);
  if (it == m_map.end())
    return std::numeric_limits<uint32_t>::max();
  return it->second;
}
