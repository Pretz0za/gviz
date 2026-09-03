#pragma once

#include <functional>
#include <limits>

#include "icomponent_pool.hpp"

template <typename Tag> class Handle {
public:
  static constexpr EntityID kInvalid = std::numeric_limits<EntityID>::max();

  constexpr Handle() noexcept : m_id(kInvalid) {}
  constexpr explicit Handle(EntityID id) noexcept : m_id(id) {}

  constexpr EntityID Raw() const noexcept { return m_id; }
  constexpr bool IsValid() const noexcept { return m_id != kInvalid; }

  friend constexpr bool operator==(Handle a, Handle b) noexcept {
    return a.m_id == b.m_id;
  }
  friend constexpr bool operator!=(Handle a, Handle b) noexcept {
    return a.m_id != b.m_id;
  }

private:
  EntityID m_id;
};

namespace std {
template <typename Tag> struct hash<Handle<Tag>> {
  size_t operator()(const Handle<Tag> &h) const noexcept {
    return std::hash<EntityID>{}(h.Raw());
  }
};
} // namespace std
