#pragma once

#include <type_traits>

/** A type-safe bitmask over a scoped enum of power-of-two values, e.g.
 *  `enum class GraphFlags : uint8_t { None = 0, Directed = 1 << 0 };`.
 *  The enum itself needs no operators -- construct a Flags<GraphFlags> from
 *  a single enumerator, combine with |, and query with Has(). */
template <typename EnumT> class Flags {
  static_assert(std::is_enum_v<EnumT>, "Flags<T> requires an enum type");

public:
  using Underlying = std::underlying_type_t<EnumT>;

  constexpr Flags() noexcept = default;
  constexpr Flags(EnumT value) noexcept
      : m_bits(static_cast<Underlying>(value)) {}

  /** True if every bit set in @p flag is also set here. */
  constexpr bool Has(EnumT flag) const noexcept {
    const Underlying bit = static_cast<Underlying>(flag);
    return (m_bits & bit) == bit;
  }

  constexpr bool Any() const noexcept { return m_bits != Underlying{0}; }

  constexpr Flags &Set(EnumT flag) noexcept {
    m_bits |= static_cast<Underlying>(flag);
    return *this;
  }

  constexpr Flags &Clear(EnumT flag) noexcept {
    m_bits &= static_cast<Underlying>(~static_cast<Underlying>(flag));
    return *this;
  }

  friend constexpr Flags operator|(Flags lhs, Flags rhs) noexcept {
    return Flags(static_cast<Underlying>(lhs.m_bits | rhs.m_bits));
  }

  friend constexpr Flags operator&(Flags lhs, Flags rhs) noexcept {
    return Flags(static_cast<Underlying>(lhs.m_bits & rhs.m_bits));
  }

  friend constexpr Flags &operator|=(Flags &lhs, Flags rhs) noexcept {
    lhs.m_bits |= rhs.m_bits;
    return lhs;
  }

  friend constexpr Flags &operator&=(Flags &lhs, Flags rhs) noexcept {
    lhs.m_bits &= rhs.m_bits;
    return lhs;
  }

  friend constexpr bool operator==(Flags lhs, Flags rhs) noexcept {
    return lhs.m_bits == rhs.m_bits;
  }

private:
  explicit constexpr Flags(Underlying bits) noexcept : m_bits(bits) {}

  Underlying m_bits{};
};
