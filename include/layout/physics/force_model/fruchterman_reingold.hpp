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

template <GraphLike G> class VanillaFruchtermanReingold {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double edgeLength = 100.0;
    // Floors the distance used for repulsion's magnitude at
    // edgeLength * minDistFraction, so two vertices landing arbitrarily
    // close together (or exactly coincident) can't spike k^2/dist into an
    // unbounded one-tick impulse. Mirrors grapher-old's VecMinDistFraction.
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

  VanillaFruchtermanReingold(DecoratedGraph<G> &dg, Configuration *config);

  Configuration *Config() { return m_config; }

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, const double* uPos, double uMass);
  inline double EdgeLength() const { return m_config->edgeLength; }

private:
  Configuration *m_config;
  uint8_t m_dimension;
  DistanceCalculationSystem<G> m_distanceCalc;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<PhysicsComponent> *m_physics;
};

template <GraphLike G> class GRIPFruchtermanReingold {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    // GRIP's tick formulas hardcode this as their implicit target spacing.
    double targetSpacing = 10.0;
    double repulsionStrength = 0.05;

    Configuration WithTargetSpacing(double v) { targetSpacing = v; return *this; }
    Configuration WithRepulsionStrength(double v) { repulsionStrength = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Target Spacing", &targetSpacing, ConfigFieldType::Double, 0.1, 200.0},
          {"Repulsion Strength", &repulsionStrength, ConfigFieldType::Double, 0.0, 5.0},
      };
    }
  };

  GRIPFruchtermanReingold(DecoratedGraph<G> &dg, Configuration *config);

  Configuration *Config() { return m_config; }

  inline void AttractiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, DenseNodeID u);
  inline void RepulsiveTick(DenseNodeID v, const double* uPos, double uMass);
  inline double EdgeLength() const { return m_config->targetSpacing; }

private:
  Configuration *m_config;
  DenseComponentPool<PhysicsComponent> *m_physics;
  DistanceCalculationSystem<G> m_distanceCalc;
  uint8_t m_dimension;
};

#include "layout/physics/force_model/fruchterman_reingold.tpp"
static_assert(ForceModel<VanillaFruchtermanReingold<Graph>>);
static_assert(ForceModel<GRIPFruchtermanReingold<Graph>>);
