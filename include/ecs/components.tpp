#pragma once

#include "ecs/components.hpp"
#include <utility>

template <typename T>
ComponentPool<T>::ComponentPool(const Admin &admin)
    : m_admin(&admin), m_local(), m_map() {}

template <typename T> T &ComponentPool<T>::Add(EntityID id) {
  m_map[id] = static_cast<uint32_t>(m_local.size());
  m_local.emplace_back();
  T &out = m_local.back();
  out.m_owner = id;
  return out;
}

template <typename T> void ComponentPool<T>::Remove(EntityID id) {
  auto it = m_map.find(id);
  if (it == m_map.end())
    return;

  uint32_t idx = it->second;
  uint32_t last = static_cast<uint32_t>(m_local.size() - 1);

  m_map[m_local[last].m_owner] = idx;
  std::swap(m_local[idx], m_local[last]);
  m_local.pop_back();
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

template <typename T> std::vector<T> &ComponentPool<T>::Data() {
  return const_cast<std::vector<T> &>(std::as_const(*this).Data());
}

template <typename T> size_t ComponentPool<T>::Size() const {
  return m_local.size();
}
