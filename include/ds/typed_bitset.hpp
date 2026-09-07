#pragma once

#include "ds/bitset.hpp"
#include "ecs/entity.hpp"
#include <cstddef>
#include <iterator>

/** A BitSet addressed by a strongly-typed index (e.g. NodeID, DenseNodeID)
 *  instead of a raw size_t, so a set built for one index space can't be
 *  tested/set with an index from another space without a compile error. */
template <typename IndexT> class TypedBitSet {
public:
  explicit TypedBitSet(size_t nbits = 0, bool fill = false)
      : m_bits(nbits, fill) {}

  size_t Size() const noexcept { return m_bits.Size(); }

  bool Test(IndexT k) const noexcept { return m_bits.Test(k.Raw()); }
  void Set(IndexT k) noexcept { m_bits.Set(k.Raw()); }
  void Clear(IndexT k) noexcept { m_bits.Clear(k.Raw()); }

  void ClearAll() noexcept { m_bits.ClearAll(); }
  void SetAll() noexcept { m_bits.SetAll(); }
  size_t Popcount() const noexcept { return m_bits.Popcount(); }
  void Resize(size_t newSize) { m_bits.Resize(newSize); }

  /** Forward iterator over set bit indices, yielding IndexT. Just adapts
   *  BitSet::iterator, so it's exactly as cheap (skips zero words, uses
   *  std::countr_zero within nonzero ones); this only adds the IndexT tag
   *  on dereference. */
  class iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = IndexT;
    using difference_type = std::ptrdiff_t;
    using pointer = const IndexT *;
    using reference = IndexT;

    iterator() = default;

    IndexT operator*() const noexcept {
      return IndexT(static_cast<EntityID>(*m_inner));
    }

    iterator &operator++() noexcept {
      ++m_inner;
      return *this;
    }

    iterator operator++(int) noexcept {
      iterator tmp = *this;
      ++m_inner;
      return tmp;
    }

    bool operator==(const iterator &other) const noexcept {
      return m_inner == other.m_inner;
    }
    bool operator!=(const iterator &other) const noexcept {
      return !(*this == other);
    }

  private:
    friend class TypedBitSet;
    explicit iterator(BitSet::iterator inner) noexcept : m_inner(inner) {}
    BitSet::iterator m_inner;
  };

  /** A begin()/end() pair over a bounded index range; see Range(). */
  class RangeView {
  public:
    iterator begin() const noexcept { return m_begin; }
    iterator end() const noexcept { return iterator(); }

  private:
    friend class TypedBitSet;
    explicit RangeView(iterator b) noexcept : m_begin(b) {}
    iterator m_begin;
  };

  iterator begin() const noexcept { return iterator(m_bits.begin()); }
  iterator end() const noexcept { return iterator(m_bits.end()); }

  /** Iterates set bits within the half-open range [start, end) only. */
  RangeView Range(IndexT start, IndexT end) const noexcept {
    return RangeView(iterator(m_bits.Range(start.Raw(), end.Raw()).begin()));
  }

private:
  BitSet m_bits;
};
