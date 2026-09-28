#pragma once

#include "ecs/components.hpp"
#include "ecs/index_space.hpp"

typedef struct MassComponent : Component {
  double mass = 1.0;
} MassComponent;

template <>
class DenseComponentPool<MassComponent> : public IDenseStorageListener {
public:
  DenseComponentPool<MassComponent>(const IndexSpace &admin) : m_admin(admin) {}

  void OnAdd() override {}

  inline MassComponent *Find(EntityID id) { return &m_out; }
  inline const MassComponent *Find(EntityID id) const { return &m_out; }

  inline std::vector<MassComponent> &Data() { return m_data; }
  inline const std::vector<MassComponent> &Data() const { return m_data; }

  uint32_t Size() const { return m_data.size(); }

  template <typename U> DenseComponentPool<U> *Sibling() const {
    return m_admin.GetPool<U>();
  }

private:
  const IndexSpace &m_admin;
  mutable MassComponent m_out{{}, 1.0};
  std::vector<MassComponent> m_data{};
};
