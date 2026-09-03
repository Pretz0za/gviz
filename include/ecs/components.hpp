#pragma once

#include <unordered_map>
#include <vector>

#include "icomponent_pool.hpp"

class Admin;

template <typename T> class ComponentPool : public IComponentPool {
public:
  ComponentPool(const Admin &admin);
  ~ComponentPool() = default;

  T &Add(EntityID id);
  void Remove(EntityID id) override;
  T *Find(EntityID id) const;
  std::vector<T> &Data() const;

private:
  std::vector<T> m_local;
  const Admin *m_admin;
  // TODO: consider how to make this an array instead
  std::unordered_map<EntityID, uint32_t> m_map;
};

struct Component {
  EntityID m_owner;
};

#include "components.tpp"
