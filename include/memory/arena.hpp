#pragma once

#include <algorithm>
#include <memory>
#include <new>
#include <numeric>
#include <span>
#include <string>
#include <utility>
#include <vector>

using std::span;
using std::string;
using std::unique_ptr;

namespace Memory {
class Arena {
public:
  constexpr explicit Arena(size_t blockCapacity) : m_blockCapacity{blockCapacity} {}

  // Note: placement-new, not std::construct_at, on purpose. construct_at's
  // constraint (and any constructible_from/is_constructible_v check) does
  // access control as if unrelated to friend declarations, so it can never
  // reach a type's private-but-friended-to-Arena constructor. A raw
  // placement-new expression is evaluated in this function's own context,
  // where Arena's friendship actually applies.
  template <typename T, typename... Args>
  constexpr T *Allocate(Args &&...args) {
    auto it = std::find_if(m_storage.begin(), m_storage.end(),
                            [](const Block &block) {
                              return block.CanAllocate(sizeof(T), alignof(T));
                            });

    Block *block = it == m_storage.end() ? &m_storage.emplace_back(m_blockCapacity)
                                          : &*it;
    void *mem = block->Allocate(sizeof(T), alignof(T));
    return ::new (mem) T(std::forward<Args>(args)...);
  }

  constexpr void Reset() noexcept {
    for (auto &block : m_storage)
      block.Reset();
  }

  constexpr size_t Remaining() const noexcept {
    return std::accumulate(
        m_storage.begin(), m_storage.end(), size_t{0},
        [](size_t acc, const Block &block) { return acc + block.Remaining(); });
  }

private:
  class Block {
  private:
    unique_ptr<std::byte[]> storage;
    size_t capacity;
    size_t offset;

  public:
    constexpr explicit Block(size_t capacity)
        : storage{std::make_unique<std::byte[]>(capacity)}, capacity{capacity},
          offset{0} {}

    constexpr ~Block() = default;
    constexpr Block(Block &&) noexcept = default;
    constexpr Block &operator=(Block &&) noexcept = default;

    constexpr Block(const Block &) = delete;
    constexpr Block &operator=(const Block &) = delete;

    constexpr void *Allocate(size_t size, size_t alignment) {
      void *ptr = storage.get() + offset;
      size_t space = capacity - offset;
      if (!std::align(alignment, size, ptr, space)) {
        throw std::bad_alloc();
      }
      offset = capacity - space + size;
      return ptr;
    }

    constexpr bool CanAllocate(size_t size, size_t alignment) const {
      void *ptr = storage.get() + offset;
      size_t space = capacity - offset;
      return std::align(alignment, size, ptr, space) != nullptr;
    }

    constexpr void Reset() noexcept { offset = 0; }

    constexpr size_t Remaining() const noexcept { return capacity - offset; }
  };

  const size_t m_blockCapacity;
  std::vector<Block> m_storage;
};
}; // namespace Memory
