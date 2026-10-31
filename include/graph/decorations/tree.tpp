#pragma once

#include "graph/decorations/components/tree_node_decorator.hpp"
#include "graph/decorations/resources/tree_certificate.hpp"
#include "graph/decorations/tree.hpp"
#include <algorithm>
#include <vector>

namespace Decorations {

template <GraphLike G> bool DecorateTree(DecoratedGraph<G> &graph, NodeID root) {
  G &g = *graph.graph;
  uint32_t n = g.Size();

  auto *parents = graph.nodeSpace.template SetPool<TreeNodeDecorator>();

  std::vector<bool> visited(n, false);
  std::vector<uint32_t> depth(n, 0);
  std::vector<NodeID> queue;
  queue.reserve(n);
  queue.push_back(root);

  DenseNodeID dRoot = g.MapToDense(root);
  visited[dRoot.Raw()] = true;
  parents->Find(dRoot.Raw())->parent = INVALID_NODE_ID;

  uint32_t visitedCount = 1;
  uint32_t totalDepth = 0;

  for (size_t i = 0; i < queue.size(); i++) {
    NodeID u = queue[i];
    DenseNodeID du = g.MapToDense(u);

    for (const AdjEntry &adj : g.OutNeighbors(u)) {
      NodeID v = adj.other;
      if (v == u)
        return false;

      DenseNodeID dv = g.MapToDense(v);
      if (visited[dv.Raw()]) {
        if (parents->Find(du.Raw())->parent == v)
          continue;
        return false;
      }

      visited[dv.Raw()] = true;
      parents->Find(dv.Raw())->parent = u;
      depth[dv.Raw()] = depth[du.Raw()] + 1;
      totalDepth = std::max(totalDepth, depth[dv.Raw()]);
      visitedCount++;
      queue.push_back(v);
    }
  }

  if (visitedCount != n)
    return false;

  graph.template SetResource<TreeCertifacteResource>(
      TreeCertifacteResource{root, totalDepth});
  return true;
}

} // namespace Decorations
