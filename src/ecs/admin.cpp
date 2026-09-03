#include "ecs/admin.hpp"
#include "ecs/icomponent_pool.hpp"

Admin::Admin() : m_freeIDs(), m_pools(), m_nextID(0) {}

EntityID Admin::CreateEntity() {
  if (m_freeIDs.empty())
    return m_nextID++;
  auto out = m_freeIDs.back();
  m_freeIDs.pop_back();
  return out;
}

void Admin::DestroyEntity(EntityID id) {
  for (const auto &[_, pool] : m_pools) {
    pool->Remove(id);
  }
  m_freeIDs.push_back(id);
}
