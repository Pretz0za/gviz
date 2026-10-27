#pragma once

#include "concept/graphLike.hpp"
#include "ecs/index_space.hpp"
#include "ecs/resource.hpp"
#include "graph/types.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

template <GraphLike G> class DecoratedGraph {
public:
  DecoratedGraph() = default;
  explicit DecoratedGraph(G &g) { Init(g); }

  void Init(G &g) {
    graph = &g;
    for (uint32_t i = 0; i < g.Size(); i++)
      nodeSpace.Create(g.MapToSparse(DenseNodeID{i}).Raw());
    for (uint32_t i = 0; i < g.EdgeCount(); i++)
      edgeSpace.Create(g.MapToSparse(DenseEdgeID{i}).Raw());
  }

  G *graph = nullptr;
  IndexSpace nodeSpace;
  IndexSpace edgeSpace;
  uint8_t dimension = 2;

  template <typename T, typename... Args> T &SetResource(Args &&...args);
  template <typename T> T *GetResource();
  template <typename T> bool HasResource() const;

private:
  std::unordered_map<std::type_index, std::unique_ptr<IResourceHolder>>
      m_resources;
};

#include "graph/decorated_graph.tpp"
