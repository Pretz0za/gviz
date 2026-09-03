#include "graph/graph.hpp"

#include "graph/components/edge.hpp"
#include "graph/components/weight.hpp"

Graph::Graph()
    : m_inAdjPool(&m_admin.GetPool<InAdjacencyComponent>()),
      m_outAdjPool(&m_admin.GetPool<OutAdjacencyComponent>()),
      m_edgePool(&m_admin.GetPool<EdgeComponent>()) {}

NodeID Graph::AddNode() {
  EntityID id = m_admin.CreateEntity();
  m_outAdjPool->Add(id);
  m_inAdjPool->Add(id);
  return NodeID(id);
}

void Graph::RemoveNode(NodeID id) {
  if (!HasNode(id))
    return;

  std::vector<AdjEntry> out = OutEdges(id);
  std::vector<AdjEntry> in = InEdges(id);
  for (const auto &entry : out)
    RemoveEdge(entry.edge);
  for (const auto &entry : in)
    RemoveEdge(entry.edge);

  m_admin.DestroyEntity(id.Raw());
}

bool Graph::HasNode(NodeID id) const {
  return m_outAdjPool->Find(id.Raw()) != nullptr;
}

EdgeID Graph::AddEdge(NodeID from, NodeID to) {
  auto *outAdj = m_outAdjPool->Find(from.Raw());
  auto *inAdj = m_inAdjPool->Find(to.Raw());
  if (!outAdj || !inAdj)
    return EdgeID{};

  EntityID id = m_admin.CreateEntity();
  auto &edge = m_edgePool->Add(id);
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
    m_admin.AddComponent<WeightComponent>(id.Raw()).value = weight;
  return id;
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

const std::vector<AdjEntry> &Graph::OutEdges(NodeID id) const {
  static const std::vector<AdjEntry> kEmpty;
  auto *adj = m_outAdjPool->Find(id.Raw());
  return adj ? adj->out : kEmpty;
}

const std::vector<AdjEntry> &Graph::InEdges(NodeID id) const {
  static const std::vector<AdjEntry> kEmpty;
  auto *adj = m_inAdjPool->Find(id.Raw());
  return adj ? adj->in : kEmpty;
}

uint32_t Graph::OutDegree(NodeID id) const {
  return static_cast<uint32_t>(OutEdges(id).size());
}

uint32_t Graph::InDegree(NodeID id) const {
  return static_cast<uint32_t>(InEdges(id).size());
}

uint32_t Graph::ToCompact(NodeID id) const {
  return m_outAdjPool->ToLocal(id.Raw());
};

uint32_t Graph::Size() const {
  return static_cast<uint32_t>(m_outAdjPool->Size());
}

Admin &Graph::Ecs() { return m_admin; }
