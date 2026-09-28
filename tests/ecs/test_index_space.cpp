// IndexSpace is an entity-id allocator with type-erased dense component
// pools attached to it. Every pool it owns gets OnAdd() called once per
// entity, whether the entity already existed when the pool was registered
// (backfill) or is created afterward (forward growth).
//
// NOTE: DenseComponentPool<T>::Sibling<U>() is NOT exercised here. Its
// return type is `DenseComponentPool<U>` (by value), but its body returns
// `m_admin.GetPool<U>()`, which is a `DenseComponentPool<U>*`. There is no
// pointer-to-object converting constructor, so instantiating Sibling<U>() at
// all fails to compile. Once components.tpp returns/derefs the pointer
// correctly, a test can be added here.

#include "catch_amalgamated.hpp"
#include "ecs/components.hpp"
#include "ecs/index_space.hpp"

#include <utility>

namespace {
struct Position2D : Component {
  double x = 0.0;
  double y = 0.0;
};

struct Tag : Component {
  int value = -1;
};
} // namespace

TEST_CASE("A fresh IndexSpace has zero entities", "[ecs][index_space]") {
  IndexSpace space;
  REQUIRE(space.Size() == 0);
}

TEST_CASE("Create allocates sequential ids and grows Size", "[ecs][index_space]") {
  IndexSpace space;
  EntityID a = space.Create();
  EntityID b = space.Create();
  EntityID c = space.Create();

  REQUIRE(a == 0u);
  REQUIRE(b == 1u);
  REQUIRE(c == 2u);
  REQUIRE(space.Size() == 3);
}

TEST_CASE("GetPool returns nullptr for a pool that was never registered",
         "[ecs][index_space]") {
  IndexSpace space;
  REQUIRE(space.GetPool<Position2D>() == nullptr);
}

TEST_CASE("SetPool registers a pool that GetPool can then retrieve",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *pool = space.SetPool<Position2D>();
  REQUIRE(pool != nullptr);
  REQUIRE(space.GetPool<Position2D>() == pool);
}

TEST_CASE("SetPool is idempotent: calling it twice returns the same pool",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *first = space.SetPool<Position2D>();
  auto *second = space.SetPool<Position2D>();
  REQUIRE(first == second);
}

TEST_CASE("Different component types get independent pools",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *positions = space.SetPool<Position2D>();
  auto *tags = space.SetPool<Tag>();
  REQUIRE(static_cast<void *>(positions) != static_cast<void *>(tags));
}

TEST_CASE("Registering a pool after entities exist backfills one slot per "
         "existing entity",
         "[ecs][index_space]") {
  IndexSpace space;
  space.Create();
  space.Create();
  space.Create();

  auto *pool = space.SetPool<Position2D>();
  REQUIRE(pool->Size() == 3);
}

TEST_CASE("Creating a new entity after a pool exists grows that pool too",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *pool = space.SetPool<Position2D>();
  REQUIRE(pool->Size() == 0);

  space.Create();
  REQUIRE(pool->Size() == 1);

  space.Create();
  space.Create();
  REQUIRE(pool->Size() == 3);
}

TEST_CASE("DenseComponentPool Set/Find round-trip a value for an entity",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *pool = space.SetPool<Position2D>();
  EntityID id = space.Create();

  pool->Set(id, Position2D{3.0, 4.0});
  Position2D *got = pool->Find(id);
  REQUIRE(got->x == 3.0);
  REQUIRE(got->y == 4.0);
}

TEST_CASE("New entities get a default-constructed component value",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *pool = space.SetPool<Tag>();
  EntityID id = space.Create();

  REQUIRE(pool->Find(id)->value == -1);
}

TEST_CASE("DenseComponentPool::Data exposes the backing storage directly",
         "[ecs][index_space]") {
  IndexSpace space;
  auto *pool = space.SetPool<Tag>();
  space.Create();
  space.Create();

  pool->Set(EntityID(0), Tag{10});
  pool->Set(EntityID(1), Tag{20});

  auto &data = pool->Data();
  REQUIRE(data.size() == 2);
  REQUIRE(data[0].value == 10);
  REQUIRE(data[1].value == 20);
}

TEST_CASE("IndexSpace is move-constructible", "[ecs][index_space]") {
  IndexSpace space;
  space.Create();
  space.SetPool<Tag>();

  IndexSpace moved(std::move(space));
  REQUIRE(moved.Size() == 1);
  REQUIRE(moved.GetPool<Tag>() != nullptr);
}
