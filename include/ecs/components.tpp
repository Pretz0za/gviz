#pragma once

#include "ecs/components.hpp"
#include <limits>
#include <utility>

template <typename T>
DenseComponentPool<T>::DenseComponentPool(IndexSpace &space) : m_space(space) {
  m_space.RegisterListener(this);
}

template <typename T> void DenseComponentPool<T>::OnAdd() {
  m_data.emplace_back();
}

template <typename T> void DenseComponentPool<T>::OnRemove(uint32_t index) {
  m_data[index] = std::move(m_data.back());
  m_data.pop_back();
}

template <typename T> const T *DenseComponentPool<T>::Find(EntityID id) const {
  uint32_t idx = m_space.CompactIndex(id);
  if (idx == std::numeric_limits<uint32_t>::max())
    return nullptr;
  return &m_data[idx];
}

template <typename T> T *DenseComponentPool<T>::Find(EntityID id) {
  return const_cast<T *>(std::as_const(*this).Find(id));
}

template <typename T> const std::vector<T> &DenseComponentPool<T>::Data() const {
  return m_data;
}

template <typename T> std::vector<T> &DenseComponentPool<T>::Data() {
  return const_cast<std::vector<T> &>(std::as_const(*this).Data());
}

template <typename T> EntityID DenseComponentPool<T>::Owner(uint32_t local) const {
  return m_space.Owner(local);
}

template <typename T> size_t DenseComponentPool<T>::Size() const {
  return m_data.size();
}

template <typename T>
uint32_t DenseComponentPool<T>::CompactIndex(EntityID id) const {
  return m_space.CompactIndex(id);
}
