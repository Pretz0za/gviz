#pragma once

// Shared helpers for wiring up a Graph's DecoratedGraph with the resources
// and component pools the layout/render systems expect to already exist.
// Every layout system looks up its pools/resources via
// GetPool<T>()/GetResource<T>() on the DecoratedGraph it's given, and either
// throws MissingResourceException or dereferences a null pointer if the
// caller hasn't set them up first -- these helpers centralize that setup so
// individual test files can stay focused on the system under test.

#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "layout/components/heat.hpp"
#include "layout/components/knearest.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/components/radius.hpp"
#include "layout/filtration/types.hpp"
#include "layout/types.hpp"

namespace test_helpers {

inline Graph MakeGraph(uint32_t nodeCount) {
  Graph g;
  for (uint32_t i = 0; i < nodeCount; i++)
    g.AddNode();
  return g;
}

inline Graph MakeDirectedGraph(uint32_t nodeCount) {
  Graph g(Graph::GraphFlags::Directed);
  for (uint32_t i = 0; i < nodeCount; i++)
    g.AddNode();
  return g;
}

inline DecoratedGraph<Graph> Decorate(Graph &g, uint8_t dimension = 2) {
  DecoratedGraph<Graph> dg(g);
  dg.dimension = dimension;
  return dg;
}

inline DenseComponentPool<PositionComponent> *
WithPositions(DecoratedGraph<Graph> &dg, uint8_t dimension) {
  return dg.nodeSpace.SetPool<PositionComponent>(dimension);
}

inline DenseComponentPool<PhysicsComponent> *WithPhysics(DecoratedGraph<Graph> &dg) {
  return dg.nodeSpace.SetPool<PhysicsComponent>();
}

inline DenseComponentPool<LocalHeatComponent> *WithLocalHeat(DecoratedGraph<Graph> &dg) {
  return dg.nodeSpace.SetPool<LocalHeatComponent>();
}

inline DenseComponentPool<RadiusComponent> *
WithRadius(DecoratedGraph<Graph> &dg, RadiusFunction fn = MANUAL) {
  return dg.nodeSpace.SetPool<RadiusComponent>(fn);
}

inline DenseComponentPool<KNearestComponent> *WithKNearest(DecoratedGraph<Graph> &dg,
                                                           uint32_t k = 32) {
  return dg.nodeSpace.SetPool<KNearestComponent>(k);
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
      : graph(MakeDirectedGraph(nodeCount)), dg(Decorate(graph, dimension)) {
    positions = WithPositions(dg, dimension);
    physics = WithPhysics(dg);
    heat = WithLocalHeat(dg);
    knearest = WithKNearest(dg);
    visible = &dg.SetResource<VisibleNodesResource>(nodeCount, 0);
    filtration = &dg.SetResource<NestedFiltrationResult>();
  }

  Graph graph;
  DecoratedGraph<Graph> dg;
  DenseComponentPool<PositionComponent> *positions;
  DenseComponentPool<PhysicsComponent> *physics;
  DenseComponentPool<LocalHeatComponent> *heat;
  DenseComponentPool<KNearestComponent> *knearest;
  VisibleNodesResource *visible;
  NestedFiltrationResult *filtration;
};

} // namespace test_helpers
