#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "ecs/components.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/concept/force_model.hpp"
#include "layout/helpers/distance.hpp"
#include <cstdint>

template <GraphLike G> class LinLog {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double edgeLength = 10.0;
    double minDistFraction = 0.01;

    Configuration WithEdgeLength(double v) { edgeLength = v; return *this; }
    Configuration WithMinDistFraction(double v) { minDistFraction = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Edge Length", &edgeLength, ConfigFieldType::Double, 1.0, 2000.0},
          {"Min Dist Fraction", &minDistFraction, ConfigFieldType::Double, 0.0, 1.0},
      };
    }
  };

  LinLog(DecoratedGraph<G> &dg, Configuration *config);

  Configuration *Config() { return m_config; }

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, const double *uPos, double uMass);
  inline double EdgeLength() const { return m_config->edgeLength; }

private:
  // 1 + degree: a vertex with no edges still repels with unit mass, and
  // every additional edge makes it push (and, in Barnes-Hut, pull) harder.
  inline double VertexMass(DenseNodeID v) const;

  Configuration *m_config;
  uint8_t m_dimension;
  DistanceCalculationSystem<G> m_distanceCalc;
  G *m_graph;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<PhysicsComponent> *m_physics;
};

#include "layout/physics/force_model/linlog.tpp"
static_assert(ForceModel<LinLog<Graph>>);
