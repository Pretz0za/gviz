#pragma once

#include <unordered_map>
#include <vector>

#include "icomponent_pool.hpp"

class Admin;

template <typename T> class ComponentPool : public IComponentPool {
public:
  ComponentPool(const Admin &admin);
  ~ComponentPool();

  T &Add(EntityID id);
  void Remove(EntityID id);
  T *Find(EntityID id) const;
  std::vector<T> &Data() const;

private:
  std::vector<T> local;
  const Admin *admin;
  // NOTE: consider how to make this an array instead
  std::unordered_map<EntityID, uint32_t> IDtoLocal;
  std::vector<EntityID> LocaltoID;
};


struct Component {
};
