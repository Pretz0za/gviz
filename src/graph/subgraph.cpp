#include "graph/subgraph.hpp"
#include "graph/components/edge.hpp"
#include "graph/types.hpp"

Subgraph::Subgraph(Graph &parent)
    : m_parent(&parent), m_compactEdgeSpace(), m_compactNodeSpace(),
      m_nodeSet(m_parent->Size(), 0), m_edgeSet(parent.EdgeCount(), 0) {
  m_degrees = NodeSpace().SetPool<DegreeComponent>();
}

DenseNodeID Subgraph::AddNode(NodeID id) {
  m_nodeSet.Set(id);
  DenseNodeID compactID = DenseNodeID(m_compactNodeSpace.Create(id.Raw()));
  setEdgesAndDegrees(id);
  m_size++;
  return compactID;
}

EdgeComponent Subgraph::GetEdge(EdgeID id) const {
  const EdgeComponent edge = m_parent->GetEdge(id);
  if (edge == INVALID_EDGE)
    return edge;
  if (HasEdge(id))
    return edge;
  return INVALID_EDGE;
}

void Subgraph::setEdgesAndDegrees(NodeID id) {
  DenseNodeID denseID = MapToDense(id);

  for (const auto &adj : OutNeighbors(id)) {
    if (HasNode(adj.other) && !m_edgeSet.Test(adj.edge)) {
      m_degrees->Find(denseID.Raw())->out++;
      m_degrees->Find(MapToDense(adj.other).Raw())->in++;
      m_edgeSet.Set(adj.edge);
      m_compactEdgeSpace.Create(adj.edge.Raw());
    }
  }

  for (const auto &adj : InNeighbors(id)) {
    if (HasNode(adj.other) && !m_edgeSet.Test(adj.edge)) {
      m_degrees->Find(denseID.Raw())->in++;
      m_degrees->Find(MapToDense(adj.other).Raw())->out++;
      m_edgeSet.Set(adj.edge);
      m_compactEdgeSpace.Create(adj.edge.Raw());
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
