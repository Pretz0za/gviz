#include "ecs/index_space.hpp"

EntityID IndexSpace::Create() {
  EntityID id = m_size++;
  for (auto &[type, pool] : m_pools)
    pool->OnAdd();
  return id;
}

size_t IndexSpace::Size() const { return m_size; }
