#pragma once

// Shared helpers for wiring up a Graph (or Subgraph) with the resources and
// component pools the layout/render systems expect to already exist. Every
// layout system looks up its pools/resources via GetPool<T>()/GetResource<T>()
// and either throws MissingResourceException or dereferences a null pointer
// if the caller hasn't set them up first -- these helpers centralize that
// setup so individual test files can stay focused on the system under test.

#include "graph/graph.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/knearest.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "layout/filtration/types.hpp"
#include "layout/types.hpp"

namespace test_helpers {

inline Graph MakeGraph(uint32_t nodeCount, uint8_t dimension = 2) {
  Graph g;
  g.SetResource<DimensionResource>(dimension == 3 ? DimensionResource::D3
                                                   : DimensionResource::D2);
  for (uint32_t i = 0; i < nodeCount; i++)
    g.AddNode();
  return g;
}

inline DenseComponentPool<PositionComponent> *
WithPositions(Graph &g, uint8_t dimension) {
  return g.NodeSpace().SetPool<PositionComponent>(dimension);
}

inline DenseComponentPool<PhysicsComponent> *WithPhysics(Graph &g) {
  return g.NodeSpace().SetPool<PhysicsComponent>();
}

inline DenseComponentPool<LocalHeatComponent> *WithLocalHeat(Graph &g) {
  return g.NodeSpace().SetPool<LocalHeatComponent>();
}

inline DenseComponentPool<RadiusComponent> *
WithRadius(Graph &g, RadiusFunction fn = MANUAL) {
  return g.NodeSpace().SetPool<RadiusComponent>(fn);
}

inline DenseComponentPool<KNearestComponent> *WithKNearest(Graph &g,
                                                           uint32_t k = 32) {
  return g.NodeSpace().SetPool<KNearestComponent>(k);
}

// Sets a node's position in-place, regardless of the graph's dimension.
inline void SetPosition(DenseComponentPool<PositionComponent> *pool,
                        uint32_t denseId, std::initializer_list<double> vals) {
  PositionComponent *c = pool->Find(denseId);
  size_t i = 0;
  for (double v : vals)
    c->pos[i++] = v;
}

// Sets up everything GRIPLayoutAlgorithm-adjacent systems need: positions,
// k-nearest storage, local heat, physics, and the two graph-wide resources
// (VisibleNodesResource + NestedFiltrationResult) that GRIPPhysicsSystem's
// constructor requires to already exist.
struct GripFixture {
  explicit GripFixture(uint32_t nodeCount, uint8_t dimension = 2)
      : graph(MakeGraph(nodeCount, dimension)) {
    positions = WithPositions(graph, dimension);
    physics = WithPhysics(graph);
    heat = WithLocalHeat(graph);
    knearest = WithKNearest(graph);
    visible = &graph.SetResource<VisibleNodesResource>(nodeCount, 0);
    filtration = &graph.SetResource<NestedFiltrationResult>();
  }

  Graph graph;
  DenseComponentPool<PositionComponent> *positions;
  DenseComponentPool<PhysicsComponent> *physics;
  DenseComponentPool<LocalHeatComponent> *heat;
  DenseComponentPool<KNearestComponent> *knearest;
  VisibleNodesResource *visible;
  NestedFiltrationResult *filtration;
};

} // namespace test_helpers
