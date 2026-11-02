#pragma once

#include "concept/graphLike.hpp"
#include "ecs/components.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/components/edge.hpp"
#include "graph/decorated_graph.hpp"
#include "graph/decorations/components/tree_node_decorator.hpp"
#include "graph/decorations/resources/tree_certificate.hpp"
#include "graph/decorations/tree.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include "graph/undirected_view.hpp"
#include <generator>
#include <stdexcept>

template <GraphLike G> class UndirectedTreeView {
public:
  UndirectedTreeView(DecoratedGraph<G> &dg, NodeID root)
      : m_view(dg.graph), m_dg(dg) {
    if (!SetRoot(root))
      throw std::runtime_error(
          "UndirectedTreeView: input graph is not a tree");
  }

  UndirectedTreeView(const UndirectedTreeView &) = delete;
  UndirectedTreeView &operator=(const UndirectedTreeView &) = delete;
  UndirectedTreeView(UndirectedTreeView &&) = delete;
  UndirectedTreeView &operator=(UndirectedTreeView &&) = delete;

  bool SetRoot(NodeID root) {
    if (!Decorations::DecorateTree(m_dg, root))
      return false;
    m_cert = m_dg.template GetResource<TreeCertificateResource>();
    m_treeDecorators = m_dg.nodeSpace.template GetPool<TreeNodeDecorator>();
    return true;
  }

  auto Nodes() const { return m_view.Nodes(); }
  auto Edges() const { return m_view.Edges(); }

  bool HasNode(NodeID id) const { return m_view.HasNode(id); }
  bool HasEdge(EdgeID id) const { return m_view.HasEdge(id); }
  EdgeComponent GetEdge(EdgeID id) const { return m_view.GetEdge(id); }
  float GetWeight(EdgeID id) const { return m_view.GetWeight(id); }

  std::generator<AdjEntry> OutNeighbors(NodeID id) const {
    for (const auto &e : m_view.OutNeighbors(id))
      co_yield e;
    co_return;
  }

  std::generator<AdjEntry> InNeighbors(NodeID id) const {
    for (const auto &e : m_view.InNeighbors(id))
      co_yield e;
    co_return;
  }

  uint32_t OutDegree(NodeID id) const { return m_view.OutDegree(id); }
  uint32_t InDegree(NodeID id) const { return m_view.InDegree(id); }
  uint32_t Degree(NodeID id) const { return m_view.Degree(id); }

  uint32_t Size() const { return m_view.Size(); }
  uint32_t EdgeCount() const { return m_view.EdgeCount(); }
  bool IsDirected() const { return false; }

  NodeID MapToSparse(DenseNodeID id) const { return m_view.MapToSparse(id); }
  DenseNodeID MapToDense(NodeID id) const { return m_view.MapToDense(id); }
  EdgeID MapToSparse(DenseEdgeID id) const { return m_view.MapToSparse(id); }
  DenseEdgeID MapToDense(EdgeID id) const { return m_view.MapToDense(id); }

  TreeCertificateResource *Certificate() const { return m_cert; }
  NodeID Root() const { return m_cert->root; }
  uint32_t TotalDepth() const { return m_cert->total_depth; }

  NodeID Parent(NodeID node) const {
    return m_treeDecorators->Find(m_view.MapToDense(node).Raw())->parent;
  }

  std::generator<AdjEntry> Children(NodeID node) const {
    NodeID parent = Parent(node);
    for (const AdjEntry &adj : m_view.OutNeighbors(node)) {
      if (adj.other == parent)
        continue;
      co_yield adj;
    }
    co_return;
  }

  uint32_t ChildCount(NodeID node) const { return m_view.OutDegree(node) - 1; }

  AdjEntry FirstChild(NodeID node) const {
    for (const AdjEntry &adj : Children(node))
      return adj;
    return {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
  }

  AdjEntry LastChild(NodeID node) const {
    AdjEntry out = {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
    for (const AdjEntry &adj : Children(node))
      out = adj;
    return out;
  }

  AdjEntry FindChild(NodeID parent, NodeID child) const {
    for (const AdjEntry &adj : Children(parent))
      if (adj.other == child)
        return adj;
    return {.edge = INVALID_EDGE_ID, .other = INVALID_NODE_ID};
  }

  bool IsLeaf(NodeID node) const { return ChildCount(node) == 0; }

  uint32_t SiblingAdjacencyDist(NodeID parent, NodeID child1,
                                 NodeID child2) const {
    uint32_t l = 0, r = 0, curr = 0;
    for (const AdjEntry &adj : Children(parent)) {
      if (adj.other == child1)
        l = curr;
      else if (adj.other == child2)
        r = curr;
      curr++;
    }
    return r - l;
  }

private:
  UndirectedView<G> m_view;
  DecoratedGraph<G> &m_dg;
  TreeCertificateResource *m_cert = nullptr;
  DenseComponentPool<TreeNodeDecorator> *m_treeDecorators = nullptr;
};

static_assert(GraphLike<UndirectedTreeView<Graph>>);
