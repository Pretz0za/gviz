// KNearestComponent's pool is a type-erased, runtime-strided store (like
// PositionComponent's): each entity gets `1 + 2*k` uint32_t slots, laid out
// as {size, then k (NodeID, depth) pairs}, reinterpreted through
// KNearestComponent's {size; FoundNode nearest[1];} layout.

#include "catch_amalgamated.hpp"
#include "ecs/index_space.hpp"
#include "layout/components/knearest.hpp"

TEST_CASE("A fresh KNearestComponent pool defaults to k=32", "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>();
  REQUIRE(pool->K() == 32);
}

TEST_CASE("The pool can be constructed with an explicit k", "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{4});
  REQUIRE(pool->K() == 4);
}

TEST_CASE("SetK updates the reported k", "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{4});
  pool->SetK(8);
  REQUIRE(pool->K() == 8);
}

TEST_CASE("Creating entities grows the pool by one slot each",
         "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{4});
  space.Create();
  space.Create();
  REQUIRE(pool->Size() == 2);
}

TEST_CASE("Find returns nullptr past the end and a valid pointer within range",
         "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{4});
  space.Create();

  REQUIRE(pool->Find(0) != nullptr);
  REQUIRE(pool->Find(1) == nullptr);
}

TEST_CASE("A new entity's slot starts zeroed", "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{4});
  space.Create();

  REQUIRE(pool->Find(0)->size == 0);
}

TEST_CASE("Writing size and nearest[] entries round-trips through Find",
         "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{4});
  space.Create();

  KNearestComponent *c = pool->Find(0);
  c->size = 2;
  c->nearest[0] = FoundNode{NodeID(5), 1};
  c->nearest[1] = FoundNode{NodeID(9), 2};

  KNearestComponent *readBack = pool->Find(0);
  REQUIRE(readBack->size == 2);
  REQUIRE(readBack->nearest[0].node == NodeID(5));
  REQUIRE(readBack->nearest[0].depth == 1);
  REQUIRE(readBack->nearest[1].node == NodeID(9));
  REQUIRE(readBack->nearest[1].depth == 2);
}

TEST_CASE("Each entity's slot is independent of its neighbors",
         "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{2});
  space.Create();
  space.Create();

  pool->Find(0)->size = 1;
  pool->Find(0)->nearest[0] = FoundNode{NodeID(1), 1};

  KNearestComponent *second = pool->Find(1);
  REQUIRE(second->size == 0);
}

TEST_CASE("Data() spans exactly Size() entities", "[layout][components][knearest]") {
  IndexSpace space;
  auto *pool = space.SetPool<KNearestComponent>(uint32_t{2});
  space.Create();
  space.Create();
  space.Create();

  auto &data = pool->Data();
  REQUIRE(data.size() == 3);

  int count = 0;
  for (auto &c : data) {
    (void)c;
    count++;
  }
  REQUIRE(count == 3);
}
