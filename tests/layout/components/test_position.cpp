// PositionComponent uses a hand-specialized DenseComponentPool with a
// runtime-configurable stride (the graph's dimension), backed by one flat
// std::vector<double> instead of one std::vector<PositionComponent>.

#include "catch_amalgamated.hpp"
#include "ecs/index_space.hpp"
#include "layout/components/position.hpp"

TEST_CASE("A position pool starts empty with the configured dimension",
         "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{3});
  REQUIRE(pool->Dimension() == 3);
  REQUIRE(pool->Size() == 0);
}

TEST_CASE("Creating entities grows the position pool by one slot each",
         "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{2});
  space.Create();
  space.Create();
  space.Create();

  REQUIRE(pool->Size() == 3);
}

TEST_CASE("Find returns nullptr past the end and a valid pointer within range",
         "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{2});
  space.Create();

  REQUIRE(pool->Find(0) != nullptr);
  REQUIRE(pool->Find(1) == nullptr);
}

TEST_CASE("New slots start zeroed and are independently writable",
         "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{3});
  space.Create();
  space.Create();

  PositionComponent *p0 = pool->Find(0);
  PositionComponent *p1 = pool->Find(1);
  REQUIRE(p0->pos[0] == 0.0);
  REQUIRE(p1->pos[0] == 0.0);

  p0->pos[0] = 1.0;
  p0->pos[1] = 2.0;
  p0->pos[2] = 3.0;

  REQUIRE(p0->pos[0] == 1.0);
  REQUIRE(p1->pos[0] == 0.0); // untouched
}

TEST_CASE("Data() exposes a PositionSpan that indexes with the same stride",
         "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{2});
  space.Create();
  space.Create();

  auto &data = pool->Data();
  REQUIRE(data.Size() == 2);

  data[0].pos[0] = 10.0;
  data[0].pos[1] = 20.0;
  data[1].pos[0] = 30.0;
  data[1].pos[1] = 40.0;

  REQUIRE(pool->Find(0)->pos[0] == 10.0);
  REQUIRE(pool->Find(1)->pos[1] == 40.0);
}

TEST_CASE("PositionSpan iteration visits every stored entity once",
         "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{2});
  space.Create();
  space.Create();
  space.Create();

  auto &data = pool->Data();
  int count = 0;
  for (auto &c : data) {
    (void)c;
    count++;
  }
  REQUIRE(count == 3);
}

TEST_CASE("SetDimension changes the reported dimension", "[layout][components][position]") {
  IndexSpace space;
  auto *pool = space.SetPool<PositionComponent>(uint8_t{2});
  pool->SetDimension(3);
  REQUIRE(pool->Dimension() == 3);
}
