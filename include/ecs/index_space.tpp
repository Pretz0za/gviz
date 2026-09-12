#pragma once

#include "ecs/index_space.hpp"
#include <memory>
#include <typeindex>

template <typename T, typename... Args>
DenseComponentPool<T> *IndexSpace::SetPool(Args &&...args) {
  auto key = std::type_index(typeid(T));
  auto it = m_pools.find(key);
  if (it == m_pools.end()) {
    auto pool =
        std::make_unique<DenseComponentPool<T>>(std::forward<Args>(args)...);
    for (uint32_t i = 0; i < m_size; i++)
      pool->OnAdd();
    auto *raw = pool.get();
    m_pools.emplace(key, std::move(pool));
    return raw;
  }
  return static_cast<DenseComponentPool<T> *>(it->second.get());
}

template <typename T> DenseComponentPool<T> *IndexSpace::GetPool() {
  auto key = std::type_index(typeid(T));
  auto it = m_pools.find(key);
  if (it == m_pools.end()) {
    return nullptr;
  }
  return static_cast<DenseComponentPool<T> *>(it->second.get());
}
