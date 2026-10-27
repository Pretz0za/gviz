#pragma once

#include "graph/decorated_graph.hpp"
#include <typeindex>
#include <utility>

template <GraphLike G>
template <typename T, typename... Args>
T &DecoratedGraph<G>::SetResource(Args &&...args) {
  auto out = GetResource<T>();
  if (out != nullptr)
    return *out;
  auto key = std::type_index(typeid(T));
  auto holder =
      std::make_unique<ResourceHolder<T>>(std::forward<Args>(args)...);
  auto *raw = holder.get();
  m_resources[key] = std::move(holder);
  return raw->m_value;
}

template <GraphLike G>
template <typename T>
T *DecoratedGraph<G>::GetResource() {
  auto key = std::type_index(typeid(T));
  auto it = m_resources.find(key);
  if (it == m_resources.end())
    return nullptr;
  return &static_cast<ResourceHolder<T> *>(it->second.get())->m_value;
}

template <GraphLike G>
template <typename T>
bool DecoratedGraph<G>::HasResource() const {
  auto key = std::type_index(typeid(T));
  return m_resources.find(key) != m_resources.end();
}
