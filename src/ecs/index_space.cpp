#include "ecs/index_space.hpp"

#include <limits>
#include <utility>

uint32_t IndexSpace::Add(EntityID id) {
  uint32_t idx = static_cast<uint32_t>(m_dense.size());
  m_sparse[id] = idx;
  m_dense.push_back(id);
  for (auto *listener : m_listeners)
    listener->OnAdd();
  return idx;
}

void IndexSpace::Remove(EntityID id) {
  auto it = m_sparse.find(id);
  if (it == m_sparse.end())
    return;

  uint32_t idx = it->second;
  uint32_t last = static_cast<uint32_t>(m_dense.size() - 1);

  for (auto *listener : m_listeners)
    listener->OnRemove(idx);

  m_sparse[m_dense[last]] = idx;
  std::swap(m_dense[idx], m_dense[last]);
  m_dense.pop_back();
  m_sparse.erase(id);
}

uint32_t IndexSpace::CompactIndex(EntityID id) const {
  auto it = m_sparse.find(id);
  if (it == m_sparse.end())
    return std::numeric_limits<uint32_t>::max();
  return it->second;
}

EntityID IndexSpace::Owner(uint32_t index) const { return m_dense[index]; }

size_t IndexSpace::Size() const { return m_dense.size(); }

void IndexSpace::RegisterListener(IDenseStorageListener *listener) {
  m_listeners.push_back(listener);
  for (size_t i = 0; i < m_dense.size(); ++i)
    listener->OnAdd();
}
