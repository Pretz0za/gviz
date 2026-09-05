#include "graph/graph.hpp"

#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"

Graph::Graph()
    : m_inAdjPool(&m_nodeSpace.GetPool<InAdjacencyComponent>()),
      m_outAdjPool(&m_nodeSpace.GetPool<OutAdjacencyComponent>()),
      m_edgePool(&m_edgeSpace.GetPool<EdgeComponent>()),
      m_weightPool(&m_edgeSpace.GetPool<WeightComponent>()) {}

NodeID Graph::AddNode() { return NodeID(m_nodeSpace.Create()); }

bool Graph::HasNode(NodeID id) const {
  return m_outAdjPool->Find(id.Raw()) != nullptr;
}

EdgeID Graph::AddEdge(NodeID from, NodeID to) {
  auto *outAdj = m_outAdjPool->Find(from.Raw());
  auto *inAdj = m_inAdjPool->Find(to.Raw());
  if (!outAdj || !inAdj)
    return EdgeID{};

  EdgeID id(m_edgeSpace.Create());
  auto &edge = *m_edgePool->Find(id.Raw());
  edge.from = from;
  edge.to = to;

  outAdj->out.push_back({id, to});
  inAdj->in.push_back({id, from});
  return id;
}

EdgeID Graph::AddEdge(NodeID from, NodeID to, float weight) {
  EdgeID id = AddEdge(from, to);
  if (id.IsValid())
    m_weightPool->Find(id.Raw())->value = weight;
  return id;
}

std::pair<EdgeID, EdgeID> Graph::AddUndirectedEdge(NodeID a, NodeID b) {
  EdgeID ab = AddEdge(a, b);
  EdgeID ba = AddEdge(b, a);
  return {ab, ba};
}

std::pair<EdgeID, EdgeID> Graph::AddUndirectedEdge(NodeID a, NodeID b,
                                                    float weight) {
  EdgeID ab = AddEdge(a, b, weight);
  EdgeID ba = AddEdge(b, a, weight);
  return {ab, ba};
}

bool Graph::HasEdge(EdgeID id) const {
  return m_edgePool->Find(id.Raw()) != nullptr;
}

NodeID Graph::Source(EdgeID id) const {
  auto *edge = m_edgePool->Find(id.Raw());
  return edge ? edge->from : NodeID{};
}

NodeID Graph::Target(EdgeID id) const {
  auto *edge = m_edgePool->Find(id.Raw());
  return edge ? edge->to : NodeID{};
}

const std::vector<AdjEntry> &Graph::OutNeighbors(NodeID id) const {
  static const std::vector<AdjEntry> kEmpty;
  auto *adj = m_outAdjPool->Find(id.Raw());
  return adj ? adj->out : kEmpty;
}

const std::vector<AdjEntry> &Graph::InNeighbors(NodeID id) const {
  static const std::vector<AdjEntry> kEmpty;
  auto *adj = m_inAdjPool->Find(id.Raw());
  return adj ? adj->in : kEmpty;
}

uint32_t Graph::OutDegree(NodeID id) const {
  return static_cast<uint32_t>(OutNeighbors(id).size());
}

uint32_t Graph::InDegree(NodeID id) const {
  return static_cast<uint32_t>(InNeighbors(id).size());
}

uint32_t Graph::ToCompact(NodeID id) const { return id.Raw(); }

uint32_t Graph::Size() const {
  return static_cast<uint32_t>(m_nodeSpace.Size());
}

IndexSpace &Graph::NodeSpace() { return m_nodeSpace; }
IndexSpace &Graph::EdgeSpace() { return m_edgeSpace; }
