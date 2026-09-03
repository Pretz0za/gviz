#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "icomponent_pool.hpp"

class Admin;

template <typename T> class ComponentPool : public IComponentPool {
public:
  ComponentPool();
  ~ComponentPool() = default;

  T &Add(EntityID id);
  void Remove(EntityID id) override;

  T *Find(EntityID id);
  const T *Find(EntityID id) const;

  std::vector<T> &Data();
  const std::vector<T> &Data() const;

  EntityID Owner(uint32_t local) const;

  size_t Size() const;

private:
  std::vector<T> m_local;
  std::vector<EntityID> m_owners;
  // TODO: consider how to make this an array instead
  std::unordered_map<EntityID, uint32_t> m_map;
};

struct Component {};

#include "components.tpp"
