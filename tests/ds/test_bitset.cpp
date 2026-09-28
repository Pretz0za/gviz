#include "catch_amalgamated.hpp"
#include "ds/bitset.hpp"

#include <vector>

TEST_CASE("A default-constructed BitSet is empty", "[ds][bitset]") {
  BitSet bs;
  REQUIRE(bs.Size() == 0);
  REQUIRE(bs.Popcount() == 0);
  REQUIRE(bs.begin() == bs.end());
}

TEST_CASE("BitSet(n) starts fully clear", "[ds][bitset]") {
  BitSet bs(100);
  REQUIRE(bs.Size() == 100);
  REQUIRE(bs.Popcount() == 0);
  for (size_t i = 0; i < 100; i++)
    REQUIRE_FALSE(bs.Test(i));
}

TEST_CASE("BitSet(n, true) starts fully set, with no stray bits past Size()",
         "[ds][bitset]") {
  BitSet bs(70, true);
  REQUIRE(bs.Popcount() == 70);
  for (size_t i = 0; i < 70; i++)
    REQUIRE(bs.Test(i));
}

TEST_CASE("Set/Clear/Test round-trip individual bits", "[ds][bitset]") {
  BitSet bs(128);
  bs.Set(0);
  bs.Set(63);
  bs.Set(64);
  bs.Set(127);

  REQUIRE(bs.Test(0));
  REQUIRE(bs.Test(63));
  REQUIRE(bs.Test(64));
  REQUIRE(bs.Test(127));
  REQUIRE_FALSE(bs.Test(1));
  REQUIRE_FALSE(bs.Test(65));
  REQUIRE(bs.Popcount() == 4);

  bs.Clear(63);
  REQUIRE_FALSE(bs.Test(63));
  REQUIRE(bs.Popcount() == 3);
}

TEST_CASE("ClearAll zeroes every bit without changing Size", "[ds][bitset]") {
  BitSet bs(50, true);
  bs.ClearAll();
  REQUIRE(bs.Size() == 50);
  REQUIRE(bs.Popcount() == 0);
}

TEST_CASE("SetAll sets every bit in range without spilling past Size",
         "[ds][bitset]") {
  BitSet bs(65);
  bs.SetAll();
  REQUIRE(bs.Popcount() == 65);
  for (size_t i = 0; i < 65; i++)
    REQUIRE(bs.Test(i));
}

TEST_CASE("Resize grows with zeroed bits and preserves existing bits",
         "[ds][bitset]") {
  BitSet bs(10);
  bs.Set(3);
  bs.Set(9);
  bs.Resize(20);

  REQUIRE(bs.Size() == 20);
  REQUIRE(bs.Test(3));
  REQUIRE(bs.Test(9));
  for (size_t i = 10; i < 20; i++)
    REQUIRE_FALSE(bs.Test(i));
}

TEST_CASE("Resize shrinking drops bits beyond the new size", "[ds][bitset]") {
  BitSet bs(70, true);
  bs.Resize(40);
  REQUIRE(bs.Size() == 40);
  REQUIRE(bs.Popcount() == 40);
}

TEST_CASE("Resize shrinking to a non-word-aligned size masks the tail word",
         "[ds][bitset]") {
  BitSet bs(128, true);
  bs.Resize(70);
  REQUIRE(bs.Popcount() == 70);
  // Growing back should not resurrect the bits that were dropped.
  bs.Resize(128);
  REQUIRE(bs.Popcount() == 70);
  for (size_t i = 70; i < 128; i++)
    REQUIRE_FALSE(bs.Test(i));
}

TEST_CASE("Iteration yields set bit indices in ascending order",
         "[ds][bitset]") {
  BitSet bs(200);
  bs.Set(5);
  bs.Set(64);
  bs.Set(63);
  bs.Set(199);

  std::vector<size_t> got(bs.begin(), bs.end());
  std::vector<size_t> expected{5, 63, 64, 199};
  REQUIRE(got == expected);
}

TEST_CASE("Iterating an all-clear BitSet yields nothing", "[ds][bitset]") {
  BitSet bs(128);
  REQUIRE(bs.begin() == bs.end());
}

TEST_CASE("Iterating a fully-set BitSet visits every index once",
         "[ds][bitset]") {
  BitSet bs(130, true);
  size_t count = 0;
  size_t expected = 0;
  for (size_t idx : bs) {
    REQUIRE(idx == expected);
    expected++;
    count++;
  }
  REQUIRE(count == 130);
}

TEST_CASE("Range(start, end) only yields set bits within [start, end)",
         "[ds][bitset]") {
  BitSet bs(100);
  bs.Set(2);
  bs.Set(10);
  bs.Set(50);
  bs.Set(90);

  std::vector<size_t> got;
  for (size_t idx : bs.Range(10, 90))
    got.push_back(idx);

  std::vector<size_t> expected{10, 50};
  REQUIRE(got == expected);
}

TEST_CASE("Range with start == end yields nothing", "[ds][bitset]") {
  BitSet bs(50, true);
  auto range = bs.Range(20, 20);
  REQUIRE(range.begin() == range.end());
}

TEST_CASE("Range spanning a whole word boundary works", "[ds][bitset]") {
  BitSet bs(200, true);
  std::vector<size_t> got;
  for (size_t idx : bs.Range(60, 68))
    got.push_back(idx);
  std::vector<size_t> expected{60, 61, 62, 63, 64, 65, 66, 67};
  REQUIRE(got == expected);
}
