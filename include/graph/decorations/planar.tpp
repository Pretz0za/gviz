#pragma once

#include "graph/decorations/components/rotation_node_decorator.hpp"
#include "graph/decorations/detail/boyer_myrvold.hpp"
#include "graph/decorations/planar.hpp"
#include "graph/decorations/resources/planar_certificate.hpp"
#include <unordered_map>
#include <utility>

namespace Decorations {

template <GraphLike G> bool DecoratePlanar(DecoratedGraph<G> &graph) {
  G &g = *graph.graph;
  uint32_t n = g.Size();

  std::vector<std::pair<uint32_t, uint32_t>> edges;
  for (NodeID u : g.Nodes()) {
    uint32_t du = g.MapToDense(u).Raw();
    for (const AdjEntry &adj : g.OutNeighbors(u)) {
      uint32_t dv = g.MapToDense(adj.other).Raw();
      if (dv > du)
        edges.push_back({du, dv});
    }
  }

  Detail::PlanarEmbeddingResult result = Detail::ComputePlanarEmbedding(n, edges);

  if (!result.planar) {
    std::vector<NodeID> vertices;
    vertices.reserve(result.kuratowskiVertices.size());
    for (uint32_t dv : result.kuratowskiVertices)
      vertices.push_back(g.MapToSparse(DenseNodeID{dv}));
    graph.template SetResource<NonPlanarCertificateResource>(
        NonPlanarCertificateResource{std::move(vertices)});
    return false;
  }

  auto *rotation = graph.nodeSpace.template SetPool<PlanarRotationAdjacencyComponent>();

  for (NodeID u : g.Nodes()) {
    DenseNodeID du = g.MapToDense(u);

    std::unordered_map<uint32_t, AdjEntry> byDenseNeighbor;
    for (const AdjEntry &adj : g.OutNeighbors(u))
      byDenseNeighbor.emplace(g.MapToDense(adj.other).Raw(), adj);

    std::vector<AdjEntry> ordered;
    ordered.reserve(result.rotation[du.Raw()].size());
    for (uint32_t dv : result.rotation[du.Raw()])
      ordered.push_back(byDenseNeighbor.at(dv));

    rotation->Find(du.Raw())->out = std::move(ordered);
  }

  return true;
}

} // namespace Decorations
