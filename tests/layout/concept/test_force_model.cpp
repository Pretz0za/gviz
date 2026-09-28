// ForceModel is a compile-time contract for pluggable force systems used by
// ForceDirectedLayoutAlgorithm. Both Fruchterman-Reingold variants already
// static_assert against it in fruchterman_reingold.hpp; this test exercises
// the concept directly as a standalone check.

#include "catch_amalgamated.hpp"
#include "graph/graph.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/physics/force_model/fruchterman_reingold.hpp"

TEST_CASE("Both Fruchterman-Reingold variants satisfy ForceModel",
         "[layout][concept][force_model]") {
  STATIC_REQUIRE(ForceModel<VanillaFruchtermanReingold<Graph>>);
  STATIC_REQUIRE(ForceModel<GRIPFruchtermanReingold<Graph>>);
}

TEST_CASE("An unrelated type does not satisfy ForceModel", "[layout][concept][force_model]") {
  struct NotAForceModel {};
  STATIC_REQUIRE_FALSE(ForceModel<NotAForceModel>);
}
