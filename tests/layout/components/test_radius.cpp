// RadiusComponent's pool supports two modes: MANUAL (an explicit, settable
// per-node radius) and a degree-derived function (MICHEALIS_MENTEN), which
// computes the radius on the fly from an entity's DegreeComponent instead of
// storing one.

#include "catch_amalgamated.hpp"
#include "ecs/index_space.hpp"
#include "graph/components/degree.hpp"
#include "layout/components/radius.hpp"

TEST_CASE("MANUAL mode starts every entity at DEFAULT_RADIUS",
         "[layout][components][radius]") {
  IndexSpace space;
  auto *pool = space.SetPool<RadiusComponent>();
  space.Create();

  REQUIRE(pool->Find(0)->radius == DEFAULT_RADIUS);
  REQUIRE(pool->Size() == 1);
}

TEST_CASE("MANUAL mode radii are independently writable per entity",
         "[layout][components][radius]") {
  IndexSpace space;
  auto *pool = space.SetPool<RadiusComponent>();
  space.Create();
  space.Create();

  pool->Find(0)->radius = 15.0;
  REQUIRE(pool->Find(0)->radius == 15.0);
  REQUIRE(pool->Find(1)->radius == DEFAULT_RADIUS);
}

TEST_CASE("MANUAL mode Data() spans every stored entity", "[layout][components][radius]") {
  IndexSpace space;
  auto *pool = space.SetPool<RadiusComponent>();
  space.Create();
  space.Create();
  space.Create();

  auto data = pool->Data();
  REQUIRE(data.size() == 3);

  int count = 0;
  for (auto &r : data) {
    (void)r;
    count++;
  }
  REQUIRE(count == 3);
}

TEST_CASE("MICHEALIS_MENTEN mode derives radius from an entity's degree",
         "[layout][components][radius]") {
  IndexSpace space;
  auto *degrees = space.SetPool<DegreeComponent>();
  auto *pool = space.SetPool<RadiusComponent>(MICHEALIS_MENTEN);
  space.Create();

  degrees->Set(0, DegreeComponent{{}, /*in=*/3, /*out=*/3});

  // total = 6, t = 6 / (6 + HALF_SATURATION_DEGREE) = 6/12 = 0.5
  // radius = DEFAULT_RADIUS + (MAX_RADIUS - DEFAULT_RADIUS) * 0.5
  double expected = DEFAULT_RADIUS + (MAX_RADIUS - DEFAULT_RADIUS) * 0.5;
  REQUIRE(pool->Find(0)->radius == Catch::Approx(expected));
}

TEST_CASE("MICHEALIS_MENTEN mode gives a zero-degree entity the minimum radius",
         "[layout][components][radius]") {
  IndexSpace space;
  auto *degrees = space.SetPool<DegreeComponent>();
  auto *pool = space.SetPool<RadiusComponent>(MICHEALIS_MENTEN);
  space.Create();
  degrees->Set(0, DegreeComponent{{}, 0, 0});

  REQUIRE(pool->Find(0)->radius == Catch::Approx(DEFAULT_RADIUS));
}

TEST_CASE("SetFunction from a degree-derived mode back to MANUAL "
         "allocates explicit storage sized to match",
         "[layout][components][radius]") {
  IndexSpace space;
  space.SetPool<DegreeComponent>();
  auto *pool = space.SetPool<RadiusComponent>(MICHEALIS_MENTEN);
  space.Create();
  space.Create();

  pool->SetFunction(MANUAL);
  REQUIRE(pool->Size() == 2);
  REQUIRE(pool->Find(0)->radius == DEFAULT_RADIUS);
}

TEST_CASE("SetFunction to MANUAL is a no-op when already MANUAL",
         "[layout][components][radius]") {
  IndexSpace space;
  auto *pool = space.SetPool<RadiusComponent>();
  space.Create();
  pool->Find(0)->radius = 42.0;

  pool->SetFunction(MANUAL);
  REQUIRE(pool->Find(0)->radius == 42.0);
}
