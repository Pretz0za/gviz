#pragma once

#include "ecs/components.hpp"
#include <utility>

template <typename T>
ComponentPool<T>::ComponentPool(const Admin &admin)
    : m_admin(&admin), m_local(), m_map() {}

template <typename T> T &ComponentPool<T>::Add(EntityID id) {
  m_map[id] = m_local.size();
  m_local.emplace_back({});
  auto out = m_local.back();
  out.m_owner = id;
  return out;
}

template <typename T> void ComponentPool<T>::Remove(EntityID id) {
  auto it = m_map.find(id);
  if (it == m_map.end())
    return;

  auto local = m_local[it->second];
  auto end = m_local.size() - 1;

  // positions will be swapped
  m_map[m_local[end].m_owner] = local;
  std::swap(m_local[local], m_local[end]);
  m_local.pop_back();
  m_map.erase(id);
}

template <typename T> T *ComponentPool<T>::Find(EntityID id) const {
  auto it = m_map.find(id);
  if (it == m_map.end())
    return nullptr;
  return m_local[it->second];
}

template <typename T> std::vector<T> &ComponentPool<T>::Data() const {
  return m_local;
}
