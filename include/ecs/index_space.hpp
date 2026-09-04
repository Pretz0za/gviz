#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "entity.hpp"

class IDenseStorageListener {
public:
  virtual ~IDenseStorageListener() = default;
  virtual void OnAdd() = 0;
  virtual void OnRemove(uint32_t index) = 0;
};

class IndexSpace {
public:
  IndexSpace() = default;

  uint32_t Add(EntityID id);
  void Remove(EntityID id);

  uint32_t CompactIndex(EntityID id) const;
  EntityID Owner(uint32_t index) const;

  size_t Size() const;

  void RegisterListener(IDenseStorageListener *listener);

private:
  std::vector<EntityID> m_dense;
  std::vector<IDenseStorageListener *> m_listeners;
  // TODO: can i make this not be a map?
  std::unordered_map<EntityID, uint32_t> m_sparse;
};
