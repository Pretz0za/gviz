#pragma once

#include "ecs/components.hpp"
#include "ecs/entity.hpp"
#include "ecs/index_space.hpp"
#include <functional>
#include <ranges>
#include <utility>

template <typename T, typename U> class ComponentPoolView {
public:
  ComponentPoolView(const IndexSpace &admin, std::function<U(const T &)> map)
      : m_pool(admin.GetPool<T>()), m_map(std::move(map)) {}
  ~ComponentPoolView() = default;

  inline U *Find(EntityID id) {
    return const_cast<U *>(std::as_const(*this).Find(id));
  }

  inline const U *Find(EntityID id) const {
    const T *src = m_pool->Find(id);
    if (!src)
      return nullptr;
    m_out = m_map(*src);
    return &m_out;
  }

  inline auto Data() const {
    return m_pool->Data() | std::views::transform(m_map);
  }

  uint32_t Size() const { return m_pool->Size(); }

private:
  DenseComponentPool<T> *m_pool;
  std::function<U(const T &)> m_map;
  mutable U m_out;
};
