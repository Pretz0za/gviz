#pragma once

#include "concept/graphLike.hpp"
#include "config/configuration.hpp"
#include "graph/types.hpp"
#include "layout/components/mass.hpp"
#include "layout/components/physics.hpp"
#include "layout/components/position.hpp"
#include "layout/components/spring.hpp"
#include "layout/components/velocity.hpp"
#include "layout/types.hpp"
#include <cstdint>

template <GraphLike G> class SpringLayoutAlgorithm {
public:
  class Configuration : public IConfiguration {
  public:
    Configuration() = default;

    double stiffness = 1.0;
    double restLength = 1.0;
    double damping = 0.5;
    double timeStep = 0.1;

    Configuration WithStiffness(double v) { stiffness = v; return *this; }
    Configuration WithRestLength(double v) { restLength = v; return *this; }
    Configuration WithDamping(double v) { damping = v; return *this; }
    Configuration WithTimeStep(double v) { timeStep = v; return *this; }

    std::vector<ConfigField> Fields() override {
      return {
          {"Stiffness", &stiffness, ConfigFieldType::Double, 0.0, 1000.0},
          {"RestLength", &restLength, ConfigFieldType::Double, 0.0, 1000.0},
          {"Damping", &damping, ConfigFieldType::Double, 0.0, 10.0},
          {"TimeStep", &timeStep, ConfigFieldType::Double, 0.0, 1.0},
      };
    }
  };

  SpringLayoutAlgorithm(G &graph, Configuration *config);

  void Tick();

  void Fix(NodeID node);
  void Unfix(NodeID node);
  bool IsFixed(NodeID node) const;
  void SetPosition(NodeID node, const double *pos);

  Configuration *Config() { return m_config; }

private:
  Configuration *m_config;
  G *m_graph;
  DenseComponentPool<PositionComponent> *m_positions;
  DenseComponentPool<VelocityComponent> *m_velocity;
  DenseComponentPool<PhysicsComponent> *m_physics;
  DenseComponentPool<MassComponent> *m_mass;
  DenseComponentPool<SpringComponent> *m_springs;
  FixedNodesResource *m_fixed;
  uint8_t m_dimension;
};

#include "layout/algorithms/spring.tpp"
