#pragma once

#include "graph/subgraph.hpp"
#include <typeindex>
#include <utility>

template <typename T, typename... Args> T &Subgraph::SetResource(Args &&...args) {
  auto key = std::type_index(typeid(T));
  auto holder = std::make_unique<ResourceHolder<T>>(std::forward<Args>(args)...);
  auto *raw = holder.get();
  m_resources[key] = std::move(holder);
  return raw->m_value;
}

template <typename T> T *Subgraph::GetResource() {
  auto key = std::type_index(typeid(T));
  auto it = m_resources.find(key);
  if (it == m_resources.end())
    return nullptr;
  return &static_cast<ResourceHolder<T> *>(it->second.get())->m_value;
}

template <typename T> bool Subgraph::HasResource() const {
  auto key = std::type_index(typeid(T));
  return m_resources.find(key) != m_resources.end();
}
