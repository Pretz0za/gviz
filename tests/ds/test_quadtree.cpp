// Unit tests for SpacialIndex::QuadTree, the Barnes-Hut style quadtree used
// by the force-directed layout to approximate long-range repulsion.

#include "catch_amalgamated.hpp"
#include "ds/quadtree.hpp"
#include <random>

using SpacialIndex::AABB;
using SpacialIndex::Point;
using SpacialIndex::QuadTree;

TEST_CASE("A default-constructed QuadTree is an empty leaf root",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree;
  REQUIRE(tree.IsRoot());
  REQUIRE(tree.IsLeaf());
  REQUIRE(tree.IsEmpty());
  REQUIRE(tree.Mass() == 0.0);
}

TEST_CASE("Insert within bounds succeeds and updates mass/center of mass",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 100.0});

  REQUIRE(tree.Insert(42, Point{10.0, 20.0}, 2.0));
  REQUIRE_FALSE(tree.IsEmpty());
  REQUIRE(tree.Mass() == Catch::Approx(2.0));

  Point com = tree.CenterOfMass();
  REQUIRE(com[0] == Catch::Approx(10.0));
  REQUIRE(com[1] == Catch::Approx(20.0));
}

TEST_CASE("Insert outside the root bounds fails and leaves the tree untouched",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 10.0});
  REQUIRE_FALSE(tree.Insert(1, Point{500.0, 500.0}));
  REQUIRE(tree.IsEmpty());
  REQUIRE(tree.Mass() == 0.0);
}

TEST_CASE("A point exactly on the boundary is considered inside",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 10.0});
  REQUIRE(tree.Insert(1, Point{10.0, 10.0}));
  REQUIRE(tree.Insert(2, Point{-10.0, -10.0}));
}

TEST_CASE("Center of mass is the mass-weighted average of inserted points",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 100.0});

  tree.Insert(1, Point{0.0, 0.0}, 1.0);
  tree.Insert(2, Point{10.0, 0.0}, 1.0);

  REQUIRE(tree.Mass() == Catch::Approx(2.0));
  Point com = tree.CenterOfMass();
  REQUIRE(com[0] == Catch::Approx(5.0));
  REQUIRE(com[1] == Catch::Approx(0.0));
}

TEST_CASE("Points stored under capacity are retrievable via PointAt/DataAt",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(7, Point{1.0, 2.0});
  tree.Insert(9, Point{3.0, 4.0});

  REQUIRE(tree.IsLeaf());

  Point p0 = tree.PointAt(0);
  Point p1 = tree.PointAt(1);
  REQUIRE(p0[0] == Catch::Approx(1.0));
  REQUIRE(p0[1] == Catch::Approx(2.0));
  REQUIRE(p1[0] == Catch::Approx(3.0));
  REQUIRE(p1[1] == Catch::Approx(4.0));

  REQUIRE(tree.DataAt(0) == 7);
  REQUIRE(tree.DataAt(1) == 9);
}

TEST_CASE("Inserting beyond capacity subdivides the node into four quadrants",
         "[ds][quadtree]") {
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});

  REQUIRE(tree.Insert(1, Point{-50.0, 50.0})); // NW
  REQUIRE(tree.IsLeaf());

  REQUIRE(tree.Insert(2, Point{50.0, 50.0})); // NE, triggers subdivision
  REQUIRE_FALSE(tree.IsLeaf());

  REQUIRE(tree.Quadrant(SpacialIndex::NW) != nullptr);
  REQUIRE(tree.Quadrant(SpacialIndex::NE) != nullptr);
  REQUIRE(tree.Quadrant(SpacialIndex::SW) != nullptr);
  REQUIRE(tree.Quadrant(SpacialIndex::SE) != nullptr);
}

TEST_CASE("After subdivision, the root's aggregate mass still equals the "
         "sum of all inserted masses",
         "[ds][quadtree]") {
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{-50.0, 50.0}, 3.0);
  tree.Insert(2, Point{50.0, 50.0}, 4.0);
  tree.Insert(3, Point{50.0, -50.0}, 5.0);

  REQUIRE(tree.Mass() == Catch::Approx(12.0));
}

TEST_CASE("Each point that causes a subdivision ends up in the correct "
         "quadrant relative to the parent center",
         "[ds][quadtree]") {
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{-50.0, 50.0});  // NW
  tree.Insert(2, Point{50.0, 50.0});   // NE
  tree.Insert(3, Point{-50.0, -50.0}); // SW
  tree.Insert(4, Point{50.0, -50.0});  // SE

  auto *nw = tree.Quadrant(SpacialIndex::NW);
  auto *ne = tree.Quadrant(SpacialIndex::NE);
  auto *sw = tree.Quadrant(SpacialIndex::SW);
  auto *se = tree.Quadrant(SpacialIndex::SE);

  REQUIRE_FALSE(nw->IsEmpty());
  REQUIRE_FALSE(ne->IsEmpty());
  REQUIRE_FALSE(sw->IsEmpty());
  REQUIRE_FALSE(se->IsEmpty());

  REQUIRE(nw->DataAt(0) == 1);
  REQUIRE(ne->DataAt(0) == 2);
  REQUIRE(sw->DataAt(0) == 3);
  REQUIRE(se->DataAt(0) == 4);
}

TEST_CASE("A child quadrant's own mass reflects the true mass of the point "
         "it holds",
         "[ds][quadtree]") {
  // Regression test for a redistribution bug: when a node subdivides,
  // ds/quadtree.tpp re-inserts its existing point(s) into the child via
  // `child->Insert(d, pt)`, silently dropping the original mass (defaulting
  // to 1.0) instead of forwarding the mass the point was originally
  // inserted with. The root's own Mass() is unaffected (it accumulates
  // mass on the way down, before recursing), but the child's mass should
  // still equal the true inserted mass once fixed.
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{-50.0, 50.0}, 9.0); // stored locally at first
  tree.Insert(2, Point{50.0, 50.0}, 1.0);  // triggers subdivision

  auto *nw = tree.Quadrant(SpacialIndex::NW);
  REQUIRE(nw->Mass() == Catch::Approx(9.0));
}

TEST_CASE("Reset on the root clears storage and installs a new bounding box",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{1.0, 1.0}, 5.0);
  REQUIRE_FALSE(tree.IsEmpty());

  tree.Reset(AABB{{10.0, 10.0}, 50.0});

  REQUIRE(tree.HalfLength() == Catch::Approx(50.0));
  Point center = tree.Center();
  REQUIRE(center[0] == Catch::Approx(10.0));
  REQUIRE(center[1] == Catch::Approx(10.0));
}

TEST_CASE("AABB::contains treats the box as closed ([-halfLength, halfLength])",
         "[ds][quadtree]") {
  AABB box{{0.0, 0.0}, 5.0};
  REQUIRE(box.contains(Point{0.0, 0.0}));
  REQUIRE(box.contains(Point{5.0, 5.0}));
  REQUIRE(box.contains(Point{-5.0, -5.0}));
  REQUIRE_FALSE(box.contains(Point{5.1, 0.0}));
  REQUIRE_FALSE(box.contains(Point{0.0, -5.1}));
}

TEST_CASE("AABB::intersects is true for overlapping boxes on both axes",
         "[ds][quadtree]") {
  AABB a{{0.0, 0.0}, 10.0};
  AABB b{{5.0, 5.0}, 10.0};
  REQUIRE(a.intersects(b));
}

TEST_CASE("AABB::intersects is false for boxes separated along the Y axis",
         "[ds][quadtree]") {
  // Regression test: AABB::intersects used to compare `center[1] -
  // center[1]` (always zero) instead of `center[1] - other.center[1]`,
  // ignoring the Y axis entirely. Two boxes that only overlap in X but are
  // far apart in Y should NOT be reported as intersecting.
  AABB a{{0.0, 0.0}, 1.0};
  AABB b{{0.0, 1000.0}, 1.0};
  REQUIRE_FALSE(a.intersects(b));
}

TEST_CASE("AABB::intersects is false for boxes separated along the X axis",
         "[ds][quadtree]") {
  AABB a{{0.0, 0.0}, 1.0};
  AABB b{{1000.0, 0.0}, 1.0};
  REQUIRE_FALSE(a.intersects(b));
}

TEST_CASE("AABB::intersects sums both boxes' half-lengths, not just the "
         "receiver's doubled",
         "[ds][quadtree]") {
  // Regression test: AABB::intersects compared against
  // `halfLength + halfLength` (the receiver's own half-length doubled)
  // instead of `halfLength + other.halfLength`. That only happens to be
  // correct when both boxes are the same size; here a large box and a
  // much smaller, clearly-separated box would be wrongly reported as
  // touching because the large box's own half-length doubled swallows the
  // gap between them.
  AABB big{{0.0, 0.0}, 50.0};
  AABB small{{200.0, 0.0}, 10.0};
  REQUIRE_FALSE(big.intersects(small));
  REQUIRE_FALSE(small.intersects(big));
}

TEST_CASE("QueryRange returns every point contained in a leaf",
         "[ds][quadtree]") {
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{1.0, 1.0});
  tree.Insert(2, Point{2.0, 2.0});
  tree.Insert(3, Point{500.0, 500.0}); // outside the tree, ignored

  auto found = tree.QueryRange(AABB{{0.0, 0.0}, 100.0});
  REQUIRE(found.size() == 2);
}

TEST_CASE("QueryRange recurses into subdivided children and gathers all of "
         "their points",
         "[ds][quadtree]") {
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{-50.0, 50.0});  // NW
  tree.Insert(2, Point{50.0, 50.0});   // NE, triggers subdivision
  tree.Insert(3, Point{-50.0, -50.0}); // SW
  tree.Insert(4, Point{50.0, -50.0});  // SE

  REQUIRE_FALSE(tree.IsLeaf());

  auto found = tree.QueryRange(AABB{{0.0, 0.0}, 100.0});
  REQUIRE(found.size() == 4);
}

TEST_CASE("QueryRange excludes points outside the query range even when "
         "they're inside the tree",
         "[ds][quadtree]") {
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{-50.0, 50.0});
  tree.Insert(2, Point{50.0, 50.0});
  tree.Insert(3, Point{-50.0, -50.0});
  tree.Insert(4, Point{50.0, -50.0});

  // Only the NW quadrant.
  auto found = tree.QueryRange(AABB{{-50.0, 50.0}, 10.0});
  REQUIRE(found.size() == 1);
  REQUIRE(found[0].data == 1);
}

TEST_CASE("Redistributing points on subdivision forwards each point's real "
         "mass to its child, not a default of 1.0",
         "[ds][quadtree]") {
  // Companion to the "child quadrant's own mass" test above: this checks
  // that a *third* insert, which forces the already-subdivided NW child to
  // subdivide again, still carries the correct mass through a second level
  // of redistribution.
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  tree.Insert(1, Point{-50.0, 25.0}, 3.0); // NW
  tree.Insert(2, Point{50.0, 50.0}, 1.0);  // NE, subdivides root
  tree.Insert(3, Point{-25.0, 75.0}, 5.0); // also NW, subdivides NW child

  auto *nw = tree.Quadrant(SpacialIndex::NW);
  REQUIRE(nw->Mass() == Catch::Approx(8.0));
}

TEST_CASE("Coincident points don't crash or hang the tree",
         "[ds][quadtree]") {
  // Regression test: since S == 1, no two points can ever share this
  // node's single storage slot, so every duplicate used to force another
  // subdivision. Because the two children of an exactly-duplicated point
  // are picked by the same rule every time, subdivision recursed forever,
  // stack-overflowing well before the depth cap below was added. Beyond
  // QuadTree::kMaxDepth, points that can't be separated are folded into
  // the leaf's aggregate mass/COM instead of recursing further.
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  for (int i = 0; i < 5000; i++) {
    REQUIRE(tree.Insert(i, Point{10.0, 10.0}, 1.0));
  }
  REQUIRE(tree.Mass() == Catch::Approx(5000.0));
}

TEST_CASE("Points closer together than double precision can resolve at the "
         "tree's scale don't crash or hang the tree",
         "[ds][quadtree]") {
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 500.0});
  for (int i = 0; i < 2000; i++) {
    Point p{1.0 + i * 1e-15, 1.0 + i * 1e-15};
    REQUIRE(tree.Insert(i, p, 1.0));
  }
  REQUIRE(tree.Mass() == Catch::Approx(2000.0));
}

TEST_CASE("A large random point cloud inserts without hitting the "
         "containment invariant",
         "[ds][quadtree]") {
  // Regression test: a point sitting exactly on the tree's outer edge (as
  // whichever point defines minX/maxX/minY/maxY of a bounding box always
  // does) could round to just outside the child QuadrantFor(p) picked for
  // it, because each subdivision re-derives a child's edge as
  // center +/- halfLength/2 instead of inheriting the parent's edge
  // value, losing a ULP or so relative to the original boundary. Seed
  // fixed for reproducibility; this seed reliably placed a point on the
  // boundary before the fix.
  std::mt19937 rng(42);
  std::uniform_real_distribution<double> dist(-100.0, 100.0);
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 100.0});
  for (int i = 0; i < 2000; i++) {
    Point p{dist(rng), dist(rng)};
    REQUIRE(tree.Insert(i, p, 1.0));
  }
}

TEST_CASE("A point exactly on the tree's own boundary still inserts after "
         "several levels of subdivision",
         "[ds][quadtree]") {
  // Forces points near the same edge the root itself sits on, so children
  // several levels down re-derive that edge from center +/- halfLength/2
  // repeatedly; this is the scenario the boundary-rounding bug above
  // needs a caller (like Helpers::GetBoundingBox) to guard against by
  // padding the box, since QuadTree itself takes whatever AABB it's given.
  QuadTree<int, 1> tree(AABB{{0.0, 0.0}, 1.0});
  REQUIRE(tree.Insert(1, Point{1.0, 1.0}));
  REQUIRE(tree.Insert(2, Point{1.0, 1.0 - 1e-10}));
  REQUIRE(tree.Insert(3, Point{1.0 - 1e-10, 1.0}));
}

TEST_CASE("Reset clears stale mass, center of mass, and point count from "
         "before the reset, not just bounds and children",
         "[ds][quadtree]") {
  // Regression test: a root small enough to never subdivide (pointCount
  // <= S) kept its old pointCount/mass/COM across Reset(), since Reset()
  // only cleared bounds and child pointers. The next Insert() would then
  // see the previous frame's leftover point as if it were still occupying
  // a storage slot, permanently corrupting the mass/COM aggregate with a
  // stale copy of one point's position from every prior frame.
  QuadTree<int, 4> tree(AABB{{0.0, 0.0}, 100.0});
  for (int tick = 0; tick < 50; tick++) {
    tree.Reset(AABB{{0.0, 0.0}, 100.0});
    REQUIRE(tree.IsEmpty());
    tree.Insert(1, Point{double(tick), double(tick)}, 1.0);
    tree.Insert(2, Point{-double(tick), -double(tick)}, 1.0);
    REQUIRE(tree.Mass() == Catch::Approx(2.0));
  }
}
