#include "ecs/admin.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include <cstdint>

class Graph {
public:
  // vertices
  NodeID AddNode();
  void RemoveNode(NodeID id);
  bool HasNode(NodeID id) const;

  EdgeID AddEdge(NodeID from, NodeID to);
  EdgeID AddEdge(NodeID from, NodeID to, float weight);
  void RemoveEdge(EdgeID id);
  bool HasEdge(EdgeID id) const;
  NodeID Source(EdgeID id) const;
  NodeID Target(EdgeID id) const;

  const std::vector<AdjEntry> &OutEdges(NodeID id) const;
  const std::vector<AdjEntry> &InEdges(NodeID id) const;
  uint32_t OutDegree(NodeID id) const;
  uint32_t InDegree(NodeID id) const;

  auto Nodes() const;
  auto Edges() const;

private:
  Admin m_admin;
};
