#include "graph/graph.hpp"

#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"

Graph::Graph()
    : m_nodeSpace(&m_admin.CreateSpace()), m_edgeSpace(&m_admin.CreateSpace()),
      m_inAdjPool(&m_admin.GetPool<InAdjacencyComponent>(*m_nodeSpace)),
      m_outAdjPool(&m_admin.GetPool<OutAdjacencyComponent>(*m_nodeSpace)),
      m_edgePool(&m_admin.GetPool<EdgeComponent>(*m_edgeSpace)),
      m_weightPool(&m_admin.GetPool<WeightComponent>(*m_edgeSpace)) {}

NodeID Graph::AddNode() {
  EntityID id = m_admin.CreateEntity();
  m_nodeSpace->Add(id);
  return NodeID(id);
}

void Graph::RemoveNode(NodeID id) {
  if (!HasNode(id))
    return;

  std::vector<AdjEntry> out = OutNeighbors(id);
  std::vector<AdjEntry> in = InNeighbors(id);
  for (const auto &entry : out)
    RemoveEdge(entry.edge);
  for (const auto &entry : in)
    RemoveEdge(entry.edge);

  m_nodeSpace->Remove(id.Raw());
  m_admin.DestroyEntity(id.Raw());
}

bool Graph::HasNode(NodeID id) const {
  return m_outAdjPool->Find(id.Raw()) != nullptr;
}

EdgeID Graph::AddEdge(NodeID from, NodeID to) {
  auto *outAdj = m_outAdjPool->Find(from.Raw());
  auto *inAdj = m_inAdjPool->Find(to.Raw());
  // NOTE: throw or assert here
  if (!outAdj || !inAdj)
    return EdgeID{};

  EntityID id = m_admin.CreateEntity();
  m_edgeSpace->Add(id);
  auto &edge = *m_edgePool->Find(id);
  edge.from = from;
  edge.to = to;

  EdgeID eid(id);
  outAdj->out.push_back({eid, to});
  inAdj->in.push_back({eid, from});
  return eid;
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

void Graph::RemoveEdge(EdgeID id) {
  auto *edge = m_edgePool->Find(id.Raw());
  if (!edge)
    return;

  NodeID from = edge->from;
  NodeID to = edge->to;

  if (auto *outAdj = m_outAdjPool->Find(from.Raw())) {
    auto &out = outAdj->out;
    std::erase_if(out, [&](const AdjEntry &e) { return e.edge == id; });
  }
  if (auto *inAdj = m_inAdjPool->Find(to.Raw())) {
    auto &in = inAdj->in;
    std::erase_if(in, [&](const AdjEntry &e) { return e.edge == id; });
  }

  m_edgeSpace->Remove(id.Raw());
  m_admin.DestroyEntity(id.Raw());
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

uint32_t Graph::ToCompact(NodeID id) const {
  return m_outAdjPool->CompactIndex(id.Raw());
}

uint32_t Graph::Size() const {
  return m_nodeSpace->Size();
}

Admin &Graph::Ecs() { return m_admin; }

IndexSpace &Graph::NodeSpace() { return *m_nodeSpace; }
IndexSpace &Graph::EdgeSpace() { return *m_edgeSpace; }
