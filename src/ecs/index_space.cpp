#include "ecs/index_space.hpp"
#include "ecs/entity.hpp"

EntityID IndexSpace::Create() {
  EntityID id = m_size++;
  for (auto &[type, pool] : m_pools)
    pool->OnAdd();
  return id;
}

EntityID IndexSpace::Create(EntityID sparseID) {
  EntityID id = m_size++;
  for (auto &[type, pool] : m_pools)
    pool->OnAdd();
  m_mapToSparse.resize(m_size);
  m_mapToSparse.back() = sparseID;
  m_mapToDense[sparseID] = id;
  return id;
}

size_t IndexSpace::Size() const { return m_size; }
