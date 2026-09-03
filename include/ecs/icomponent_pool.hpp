#pragma once

#include <cstdint>

using EntityID = uint32_t;

class IComponentPool {
public:
  virtual ~IComponentPool() = default;
  virtual void
  Remove(EntityID id) = 0; // called during entity destruction, no T known
  virtual bool Contains(EntityID id) const = 0;
};
