#include "graph/graph.hpp"

#include "ds/flags.hpp"
#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"
#include "graph/types.hpp"
#include <cstdint>

Graph::Graph(Flags<Graph::GraphFlags> flags)
    : m_inAdjPool(nullptr),
      m_outAdjPool(m_nodeSpace.SetPool<OutAdjacencyComponent>()),
      m_degreePool(m_nodeSpace.SetPool<DegreeComponent>()),
      m_edgePool(m_edgeSpace.SetPool<EdgeComponent>()),
      m_weightPool(m_edgeSpace.SetPool<WeightComponent>()), m_flags(flags) {

  if (m_flags.Has(GraphFlags::Directed))
    m_inAdjPool = m_nodeSpace.SetPool<InAdjacencyComponent>();
}

NodeID Graph::AddNode() {
  m_version++;
  return NodeID(m_nodeSpace.Create());
}

bool Graph::HasNode(NodeID id) const {
  return m_outAdjPool->Find(id.Raw()) != nullptr;
}

EdgeID Graph::AddEdge(NodeID from, NodeID to) {
  m_version++;

  auto *fromAdj = m_outAdjPool->Find(from.Raw());

  EdgeID id(m_edgeSpace.Create());
  auto &edge = *m_edgePool->Find(id.Raw());
  edge.from = from;
  edge.to = to;

  fromAdj->out.push_back({id, to});
  m_degreePool->Find(from.Raw())->out++;

  if (!m_flags.Has(GraphFlags::Directed)) {
    auto *toAdj = m_outAdjPool->Find(to.Raw());

    toAdj->out.push_back({id, from});
    m_degreePool->Find(to.Raw())->out++;
  } else {
    auto *inAdj = m_inAdjPool->Find(to.Raw());

    inAdj->in.push_back({id, from});
    m_degreePool->Find(to.Raw())->in++;
  }

  return id;
}

EdgeID Graph::AddEdge(NodeID from, NodeID to, float weight) {
  EdgeID id = AddEdge(from, to);
  if (id.IsValid())
    m_weightPool->Find(id.Raw())->value = weight;
  return id;
}

bool Graph::HasEdge(EdgeID id) const {
  return m_edgePool->Find(id.Raw()) != nullptr;
}

EdgeComponent Graph::GetEdge(EdgeID id) const {
  auto *edge = m_edgePool->Find(id.Raw());
  return edge ? *edge : INVALID_EDGE;
}

float Graph::GetWeight(EdgeID id) const {
  auto *weight = m_weightPool->Find(id.Raw());
  return weight ? weight->value : 1.0f;
}

const std::vector<AdjEntry> &Graph::OutNeighbors(NodeID id) const {
  static const std::vector<AdjEntry> kEmpty;
  auto *adj = m_outAdjPool->Find(id.Raw());
  return adj ? adj->out : kEmpty;
}

const std::vector<AdjEntry> &Graph::InNeighbors(NodeID id) const {
  static const std::vector<AdjEntry> kEmpty;
  if (!m_inAdjPool)
    return kEmpty;
  auto *adj = m_inAdjPool->Find(id.Raw());
  return adj ? adj->in : kEmpty;
}

uint32_t Graph::OutDegree(NodeID id) const {
  return static_cast<uint32_t>(OutNeighbors(id).size());
}

uint32_t Graph::InDegree(NodeID id) const {
  return static_cast<uint32_t>(InNeighbors(id).size());
}

uint32_t Graph::Degree(NodeID id) const { return InDegree(id) + OutDegree(id); }

uint32_t Graph::Size() const {
  return static_cast<uint32_t>(m_nodeSpace.Size());
}
