#pragma once

#include "ecs/components.hpp"
#include "ecs/entity.hpp"
#include <utility>

template <typename T> void DenseComponentPool<T>::OnAdd() {
  m_data.emplace_back();
}

template <typename T> void DenseComponentPool<T>::Set(EntityID id, T data) {
	m_data[id] = data;
}

template <typename T> const T *DenseComponentPool<T>::Find(EntityID id) const {
  if (id >= m_data.size())
    return nullptr;
  return &m_data[id];
}

template <typename T> T *DenseComponentPool<T>::Find(EntityID id) {
  return const_cast<T *>(std::as_const(*this).Find(id));
}

template <typename T>
const std::vector<T> &DenseComponentPool<T>::Data() const {
  return m_data;
}

template <typename T> std::vector<T> &DenseComponentPool<T>::Data() {
  return const_cast<std::vector<T> &>(std::as_const(*this).Data());
}

template <typename T> size_t DenseComponentPool<T>::Size() const {
  return m_data.size();
}
