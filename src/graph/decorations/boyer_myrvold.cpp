#include "graph/decorations/detail/boyer_myrvold.hpp"

#include "boyerMyrvold/appconst.h"
#include "boyerMyrvold/graph.h"

#include <algorithm>
#include <new>
#include <stdexcept>

namespace {

void AdjacencyFromBoyer(graphP theGraph, int v, std::vector<uint32_t> &out) {
  out.clear();
  int N = theGraph->N;
  int J = theGraph->G[v].link[1];
  while (J >= N) {
    int curr = theGraph->G[J].v;
    if (curr >= 0 && curr < N)
      out.push_back(static_cast<uint32_t>(curr));
    J = theGraph->G[J].link[1];
  }
  std::reverse(out.begin(), out.end());
}

} // namespace

namespace Decorations::Detail {

PlanarEmbeddingResult
ComputePlanarEmbedding(uint32_t vertexCount,
                       const std::vector<std::pair<uint32_t, uint32_t>> &edges) {
  graphP boyer = gp_New();
  if (!boyer)
    throw std::bad_alloc();

  if (gp_InitGraph(boyer, static_cast<int>(vertexCount)) != OK) {
    gp_Free(&boyer);
    throw std::runtime_error("ComputePlanarEmbedding: failed to initialize Boyer-Myrvold graph");
  }

  for (const auto &edge : edges) {
    if (gp_AddEdge(boyer, static_cast<int>(edge.first), 0, static_cast<int>(edge.second), 0) !=
        OK) {
      gp_Free(&boyer);
      throw std::runtime_error(
          "ComputePlanarEmbedding: failed to add edge to Boyer-Myrvold graph");
    }
  }

  int res = gp_Embed(boyer, 0);
  if (res != OK && res != NONPLANAR) {
    gp_Free(&boyer);
    throw std::runtime_error("ComputePlanarEmbedding: Boyer-Myrvold embedding failed");
  }

  PlanarEmbeddingResult result;
  result.planar = res == OK;

  if (result.planar) {
    if (gp_SortVertices(boyer) != OK) {
      gp_Free(&boyer);
      throw std::runtime_error("ComputePlanarEmbedding: Boyer-Myrvold vertex sort failed");
    }

    result.rotation.resize(vertexCount);
    for (uint32_t v = 0; v < vertexCount; v++)
      AdjacencyFromBoyer(boyer, static_cast<int>(v), result.rotation[v]);
  } else {
    std::vector<uint32_t> adj;
    for (uint32_t v = 0; v < vertexCount; v++) {
      AdjacencyFromBoyer(boyer, static_cast<int>(v), adj);
      if (!adj.empty())
        result.kuratowskiVertices.push_back(v);
    }
  }

  gp_Free(&boyer);
  return result;
}

} // namespace Decorations::Detail
