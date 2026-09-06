#include "graph/subgraph.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"

Subgraph::Subgraph(Graph &parent)
    : m_parent(&parent), m_nodeSet(m_parent->Size(), 0),
      m_mapToDense{m_parent->Size(), INVALID_NODE_ID}, m_compactNodeSpace() {}

void Subgraph::AddNode(NodeID id) {
  m_nodeSet.Set(id.Raw());
  NodeID compactID = NodeID(m_compactNodeSpace.Create());
  m_mapToDense[id.Raw()] = compactID;
  m_mapToSparse.push_back(id);
  m_size++;
}

bool Subgraph::HasNode(NodeID id) const {
  return m_nodeSet.Test(static_cast<size_t>(id.Raw()));
}

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

// TODO:
uint32_t Subgraph::OutDegree(NodeID id) const { return 0xFFFFFFFF; }
uint32_t Subgraph::InDegree(NodeID id) const { return 0xFFFFFFFF; }

// auto Subgraph::Edges() const {}

IndexSpace &Subgraph::NodeSpace() { return m_compactNodeSpace; }
// IndexSpace &Subgraph::EdgeSpace() {}
