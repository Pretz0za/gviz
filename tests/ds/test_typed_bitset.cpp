// TypedBitSet<IndexT> is a thin adapter over BitSet that swaps raw size_t
// indices for a strong Handle<Tag> type. We exercise it through the
// graph/types.hpp aliases (SparseNodeSet / DenseNodeSet) since those are how
// the rest of the codebase actually uses it.

#include "catch_amalgamated.hpp"
#include "graph/types.hpp"

#include <type_traits>
#include <vector>

TEST_CASE("TypedBitSet Set/Test/Clear round-trip via NodeID", "[ds][typed_bitset]") {
  SparseNodeSet set(10);
  NodeID a(2);
  NodeID b(7);

  REQUIRE_FALSE(set.Test(a));
  set.Set(a);
  set.Set(b);
  REQUIRE(set.Test(a));
  REQUIRE(set.Test(b));
  REQUIRE(set.Popcount() == 2);

  set.Clear(a);
  REQUIRE_FALSE(set.Test(a));
  REQUIRE(set.Popcount() == 1);
}

TEST_CASE("TypedBitSet ClearAll/SetAll affect every indexed bit",
         "[ds][typed_bitset]") {
  DenseNodeSet set(16);
  set.SetAll();
  REQUIRE(set.Popcount() == 16);
  set.ClearAll();
  REQUIRE(set.Popcount() == 0);
}

TEST_CASE("TypedBitSet Resize preserves Size() semantics", "[ds][typed_bitset]") {
  SparseNodeSet set(4);
  set.Set(NodeID(1));
  set.Resize(8);
  REQUIRE(set.Size() == 8);
  REQUIRE(set.Test(NodeID(1)));
}

TEST_CASE("TypedBitSet iteration yields IndexT values in ascending raw order",
         "[ds][typed_bitset]") {
  DenseNodeSet set(20);
  set.Set(DenseNodeID(3));
  set.Set(DenseNodeID(9));
  set.Set(DenseNodeID(15));

  std::vector<uint32_t> got;
  for (DenseNodeID id : set)
    got.push_back(id.Raw());

  std::vector<uint32_t> expected{3, 9, 15};
  REQUIRE(got == expected);
}

TEST_CASE("TypedBitSet Range filters iteration to [start, end) by raw index",
         "[ds][typed_bitset]") {
  DenseNodeSet set(30);
  set.Set(DenseNodeID(1));
  set.Set(DenseNodeID(10));
  set.Set(DenseNodeID(20));

  std::vector<uint32_t> got;
  for (DenseNodeID id : set.Range(DenseNodeID(5), DenseNodeID(25)))
    got.push_back(id.Raw());

  std::vector<uint32_t> expected{10, 20};
  REQUIRE(got == expected);
}

TEST_CASE("Different Handle tags produce distinct TypedBitSet types",
         "[ds][typed_bitset]") {
  // This is a compile-time property: SparseNodeSet and DenseNodeSet are
  // different instantiations, so a NodeID can't be used to index a
  // DenseNodeSet and vice versa. We can't assert a compile error at runtime,
  // but we can assert the types genuinely differ.
  STATIC_REQUIRE_FALSE(std::is_same_v<SparseNodeSet, DenseNodeSet>);
}
