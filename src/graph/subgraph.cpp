#include "graph/subgraph.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"

Subgraph::Subgraph(Graph &parent)
    : m_parent(&parent), m_compactNodeSpace(), m_nodeSet(m_parent->Size(), 0),
      m_mapToDense{m_parent->Size(), INVALID_DENSE_NODE_ID} {
  m_degrees = NodeSpace().SetPool<DegreeComponent>();
}

void Subgraph::AddNode(NodeID id) {
  m_nodeSet.Set(id);
  DenseNodeID compactID = DenseNodeID(m_compactNodeSpace.Create());
  m_mapToDense[id.Raw()] = compactID;
  m_mapToSparse.push_back(id);
  incrementDegrees(id);
  m_size++;
}

bool Subgraph::HasNode(NodeID id) const { return m_nodeSet.Test(id); }

bool Subgraph::HasEdge(EdgeID id) const {
  const EdgeComponent edge = m_parent->GetEdge(id);
  if (edge == INVALID_EDGE)
    return false;
  return HasNode(edge.from) && HasNode(edge.to);
}

EdgeComponent Subgraph::GetEdge(EdgeID id) const {
  const EdgeComponent edge = m_parent->GetEdge(id);
  if (edge == INVALID_EDGE)
    return edge;
  if (HasNode(edge.from) && HasNode(edge.to))
    return edge;
  return INVALID_EDGE;
}

void Subgraph::incrementDegrees(NodeID id) {
  DenseNodeID denseID = MapToDense(id);
  for (auto adj : m_parent->OutNeighbors(id)) {
    if (HasNode(adj.other)) {
      m_degrees->Find(denseID.Raw())->out++;
      m_degrees->Find(MapToDense(adj.other).Raw())->in++;
    }
  }

  for (auto adj : m_parent->InNeighbors(id)) {
    if (HasNode(adj.other)) {
      m_degrees->Find(denseID.Raw())->in++;
      m_degrees->Find(MapToDense(adj.other).Raw())->out++;
    }
  }
}

  uint32_t Subgraph::OutDegree(NodeID id) const {
  auto *res = m_degrees->Find(MapToDense(id).Raw());
  return res ? res->out : 0;
}
uint32_t Subgraph::InDegree(NodeID id) const {
  auto *res = m_degrees->Find(MapToDense(id).Raw());
  return res ? res->in : 0;
}

uint32_t Subgraph::Degree(NodeID id) const {
  auto *res = m_degrees->Find(MapToDense(id).Raw());
  return res ? res->in + res->out : 0;
}

IndexSpace &Subgraph::NodeSpace() { return m_compactNodeSpace; }
// IndexSpace &Subgraph::EdgeSpace() {}
