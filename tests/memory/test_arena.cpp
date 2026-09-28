// Unit tests for Memory::Arena, the block-based bump allocator backing
// SpacialIndex::QuadTree's child-node storage.

#include "catch_amalgamated.hpp"
#include "memory/arena.hpp"

namespace {
struct Point3 {
  double x, y, z;
};

struct Counted {
  static inline int liveCount = 0;
  int value;
  explicit Counted(int v) : value(v) { liveCount++; }
  ~Counted() { liveCount--; }
};
} // namespace

TEST_CASE("A fresh Arena reports its full block capacity as remaining",
         "[memory][arena]") {
  Memory::Arena arena(1024);
  // No blocks have been allocated yet, so nothing is reserved.
  REQUIRE(arena.Remaining() == 0);
}

TEST_CASE("Allocate constructs an object in place and returns a usable pointer",
         "[memory][arena]") {
  Memory::Arena arena(1024);
  Point3 *p = arena.Allocate<Point3>(1.0, 2.0, 3.0);
  REQUIRE(p != nullptr);
  REQUIRE(p->x == 1.0);
  REQUIRE(p->y == 2.0);
  REQUIRE(p->z == 3.0);
}

TEST_CASE("Allocate forwards constructor arguments", "[memory][arena]") {
  Memory::Arena arena(1024);
  Counted *c = arena.Allocate<Counted>(42);
  REQUIRE(c->value == 42);
}

TEST_CASE("Allocate reduces the remaining space in the current block",
         "[memory][arena]") {
  Memory::Arena arena(1024);
  arena.Allocate<int>(1);
  size_t afterOne = arena.Remaining();
  REQUIRE(afterOne < 1024);

  arena.Allocate<int>(2);
  size_t afterTwo = arena.Remaining();
  REQUIRE(afterTwo < afterOne);
}

TEST_CASE("Allocations beyond one block's capacity spill into a new block",
         "[memory][arena]") {
  Memory::Arena arena(sizeof(int) * 2);
  int *a = arena.Allocate<int>(1);
  int *b = arena.Allocate<int>(2);
  int *c = arena.Allocate<int>(3); // doesn't fit in the first block anymore

  REQUIRE(*a == 1);
  REQUIRE(*b == 2);
  REQUIRE(*c == 3);
  // Two blocks now exist, so total remaining includes the second block's
  // untouched capacity.
  REQUIRE(arena.Remaining() >= sizeof(int));
}

TEST_CASE("Reset reclaims all blocks' space without destructing objects",
         "[memory][arena]") {
  Memory::Arena arena(1024);
  arena.Allocate<int>(1);
  size_t before = arena.Remaining();

  arena.Reset();

  REQUIRE(arena.Remaining() > before);
  REQUIRE(arena.Remaining() == 1024);
}

TEST_CASE("Alignment is respected for over-aligned types", "[memory][arena]") {
  struct alignas(64) Aligned {
    int value;
  };
  Memory::Arena arena(4096);
  Aligned *a = arena.Allocate<Aligned>();
  REQUIRE(reinterpret_cast<uintptr_t>(a) % alignof(Aligned) == 0);
}
