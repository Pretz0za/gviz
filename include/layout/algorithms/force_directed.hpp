#pragma once

#include "concept/graphLike.hpp"
#include "debug/chart_recorder.hpp"
#include "ds/quadtree.hpp"
#include "layout/algorithms/types.hpp"
#include "layout/components/mass.hpp"
#include "layout/components/position.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/heat/force_atlas.hpp"
#include "layout/physics/gravity.hpp"
#include "layout/placement/randomized.hpp"
#include <cstdint>
#include <memory>

template <GraphLike G, ForceModel F> class ForceDirectedLayoutAlgorithm {

public:
  ForceDirectedLayoutAlgorithm(G &graph);
  void Tick();

private:
  void RepulsiveTick(SpacialIndex::QuadTree<DenseNodeID, 1> *node, DenseNodeID v);
  void RebuildQuadTree();
  G *m_graph;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<MassComponent> *m_mass;
  std::unique_ptr<F> m_forceModel;
  std::unique_ptr<GravityForceSystem<G>> m_gravity;
  std::unique_ptr<PositionRandomized<G>> m_randomizer;
  std::unique_ptr<ForceAtlasHeat<G>> m_heat;
  QuadTreeResource *m_quadtree;
  uint8_t m_dimension;
#ifdef GVIZ_DEBUG_CHARTS
  ChartRecorder *m_chart = nullptr;
#endif
};

#include "layout/algorithms/force_directed.tpp"
