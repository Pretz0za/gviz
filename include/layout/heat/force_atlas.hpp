#pragma once

#include "concept/graphLike.hpp"
#include "graph/types.hpp"
#include "layout/components/force_atlas_heat.hpp"

// Global adaptive-speed control from Jacomy et al., "ForceAtlas2, a
// Continuous Graph Layout Algorithm for Handy Network Visualization
// Designed for the Gephi Software" (PLOS ONE, 2014), section "Speed
// regulation". Where LocalHeatGrip cools each node off its own force
// history, this tracks one global speed from how much the whole graph is
// swinging (oscillating) vs pulling steadily (traction), then damps each
// node by its own swinging.
template <GraphLike G> class ForceAtlasHeat {
public:
  ForceAtlasHeat(G &graph);

  void BeginTick();
  void Observe(DenseNodeID id, const double *force);
  void UpdateGlobalSpeed(uint32_t nodeCount);
  double SpeedFactor(DenseNodeID id) const;

private:
  DenseComponentPool<ForceAtlasHeatComponent> *m_heatPool;
  uint8_t m_dimension;

  double m_totalSwinging = 0.0;
  double m_totalTraction = 0.0;
  double m_globalSpeed = 1.0;
  double m_speedEfficiency = 1.0;
  double m_jitterTolerance = 1.0;
};

#include "layout/heat/force_atlas.tpp"
