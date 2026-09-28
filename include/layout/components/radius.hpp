#pragma once

#include "ecs/component_view.hpp"
#include "ecs/components.hpp"
#include "graph/components/degree.hpp"
#include <ranges>
#include <utility>

#define DEFAULT_RADIUS 16.0
#define MAX_RADIUS 32.0
#define HALF_SATURATION_DEGREE 6.0

typedef struct RadiusComponent : Component {
  double radius = DEFAULT_RADIUS;
} RadiusComponent;

enum RadiusFunction {
  MICHEALIS_MENTEN,
  MANUAL,
};

template <>
class DenseComponentPool<RadiusComponent> : public IDenseStorageListener {
public:
  DenseComponentPool(const IndexSpace &admin, RadiusFunction function = MANUAL)
      : m_data(), m_function(function), m_degreeView(admin, michaelisMenten) {}
  ~DenseComponentPool() override = default;

  void SetFunction(RadiusFunction target) {
    if (target == MANUAL) {
      if (m_function == MANUAL)
        return;
      m_data.resize(m_degreeView.Size());
    }
    m_function = target;
  }

  void OnAdd() override {
    if (m_function == MANUAL) {
      m_data.emplace_back();
    }
  }

  inline RadiusComponent *Find(EntityID id) {
    return const_cast<RadiusComponent *>(std::as_const(*this).Find(id));
  }

  inline const RadiusComponent *Find(EntityID id) const {
    if (m_function == MANUAL)
      return &m_data[id];
    return m_degreeView.Find(id);
  }

  inline auto Data() {
    return std::views::iota(EntityID{0}, static_cast<EntityID>(Size())) |
           std::views::transform(
               [this](EntityID id) -> RadiusComponent & { return *Find(id); });
  }

  size_t Size() const {
    if (m_function == MANUAL)
      return m_data.size();
    return m_degreeView.Size();
  }

private:
  static RadiusComponent michaelisMenten(const DegreeComponent &degree) {
    double total =
        static_cast<double>(degree.in) + static_cast<double>(degree.out);
    double t = total / (total + HALF_SATURATION_DEGREE);
    RadiusComponent radius;
    radius.radius = DEFAULT_RADIUS + (MAX_RADIUS - DEFAULT_RADIUS) * t;
    return radius;
  }

  std::vector<RadiusComponent> m_data;
  RadiusFunction m_function;
  ComponentPoolView<DegreeComponent, RadiusComponent> m_degreeView;
};
