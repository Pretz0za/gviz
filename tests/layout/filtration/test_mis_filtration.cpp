// MisFiltrationSystem builds GRIP's nested sequence of layers: layer 0 is
// the whole graph, and each subsequent layer is a maximal-independent-set-
// like subset built by greedily keeping nodes and marking everything within
// a growing BFS radius (radius = 2^(layer-1)) as covered, stopping once a
// layer shrinks to dimension+1 nodes or fewer.
//
// The expected values below were hand-traced for a 6-node undirected path
// graph (0-1-2-3-4-5) at dimension=2 (stop threshold = 3 nodes):
//
//   Layer 0 (radius doesn't apply): all 6 nodes.
//   Layer 1 (radius=1): scanning nodes 0..5 in order, greedily keep a node
//   if it isn't already marked, then mark its direct neighbors as covered.
//     node 0 -> kept, marks {1}
//     node 1 -> already marked, skipped
//     node 2 -> kept, marks {1, 3}
//     node 3 -> already marked, skipped
//     node 4 -> kept, marks {1, 3, 5}
//     node 5 -> already marked, skipped
//   Layer 1 = {0, 2, 4}, size 3 == dimension+1, so filtration stops there.
//
// m_filtration ends up holding, in order: the final layer's nodes first
// ([0, 2, 4]), then layer 0's dropped nodes filled in backwards from the
// end of the array ([..., 5, 3, 1]), for a full array of [0, 2, 4, 5, 3, 1].
// m_borders = [6, 3] (border[1] is forced to dimension+1 regardless of the
// natural count, "to ensure the last layer has enough for a simplex").

#include "catch_amalgamated.hpp"
#include "layout/filtration/mis_filtration.hpp"

#include "helpers/graph_helpers.hpp"

#include <vector>

using test_helpers::MakeGraph;

namespace {
Graph MakePathGraph(uint32_t length) {
  Graph g = MakeGraph(length, 2);
  for (uint32_t i = 0; i + 1 < length; i++)
    g.AddUndirectedEdge(NodeID(i), NodeID(i + 1));
  return g;
}
} // namespace

TEST_CASE("Tick produces the hand-traced two-layer filtration for a 6-node path",
         "[layout][filtration][mis]") {
  Graph g = MakePathGraph(6);

  MisFiltrationSystem<Graph> system(g);
  system.Tick();

  NestedFiltrationResult *result = g.GetResource<NestedFiltrationResult>();
  REQUIRE(result != nullptr);
  REQUIRE(result->m_layerCount == 2);
  REQUIRE(result->m_borders.size() == 2);
  REQUIRE(result->m_borders[0] == 6);
  REQUIRE(result->m_borders[1] == 3);

  REQUIRE(result->m_filtration.size() == 6);
  REQUIRE(result->m_filtration[0] == DenseNodeID(0));
  REQUIRE(result->m_filtration[1] == DenseNodeID(2));
  REQUIRE(result->m_filtration[2] == DenseNodeID(4));
  REQUIRE(result->m_filtration[3] == DenseNodeID(5));
  REQUIRE(result->m_filtration[4] == DenseNodeID(3));
  REQUIRE(result->m_filtration[5] == DenseNodeID(1));
}

TEST_CASE("The coarsest layer always has at least dimension+1 border width",
         "[layout][filtration][mis]") {
  Graph g = MakePathGraph(6);
  MisFiltrationSystem<Graph> system(g);
  system.Tick();

  NestedFiltrationResult *result = g.GetResource<NestedFiltrationResult>();
  uint32_t lastBorder = result->m_borders[result->m_layerCount - 1];
  REQUIRE(lastBorder >= 3); // dimension(2) + 1
}

TEST_CASE("Calling Tick again rebuilds the filtration from scratch",
         "[layout][filtration][mis]") {
  Graph g = MakePathGraph(6);
  MisFiltrationSystem<Graph> system(g);

  system.Tick();
  NestedFiltrationResult *first = g.GetResource<NestedFiltrationResult>();
  uint32_t firstLayerCount = first->m_layerCount;

  system.Tick();
  NestedFiltrationResult *second = g.GetResource<NestedFiltrationResult>();

  REQUIRE(second == first); // same resource object, rebuilt in place
  REQUIRE(second->m_layerCount == firstLayerCount);
  REQUIRE(second->m_borders[0] == 6);
}

TEST_CASE("The union of every layer's nodes covers the whole graph exactly once",
         "[layout][filtration][mis]") {
  Graph g = MakePathGraph(6);
  MisFiltrationSystem<Graph> system(g);
  system.Tick();

  NestedFiltrationResult *result = g.GetResource<NestedFiltrationResult>();
  REQUIRE(result->m_filtration.size() == g.Size());

  std::vector<bool> seen(g.Size(), false);
  for (DenseNodeID id : result->m_filtration) {
    REQUIRE_FALSE(seen[id.Raw()]);
    seen[id.Raw()] = true;
  }
  for (bool s : seen)
    REQUIRE(s);
}
