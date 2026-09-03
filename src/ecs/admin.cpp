#include "ecs/admin.hpp"

Admin::Admin() : m_freeIDs(), m_spaces(), m_pools(), m_resources(), m_nextID(0) {}

EntityID Admin::CreateEntity() {
  if (m_freeIDs.empty())
    return m_nextID++;
  auto out = m_freeIDs.back();
  m_freeIDs.pop_back();
  return out;
}

void Admin::DestroyEntity(EntityID id) { m_freeIDs.push_back(id); }

IndexSpace &Admin::CreateSpace() {
  m_spaces.emplace_back();
  return m_spaces.back();
}
