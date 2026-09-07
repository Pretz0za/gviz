#pragma once

#include "ecs/components.hpp"
#include "ecs/exceptions.hpp"
#include "graph/types.hpp"
#include "layout/algorithms/grip.hpp"
#include "layout/components/position.hpp"
#include "layout/filtration/mis_filtration.hpp"
#include <cstdio>

template <GraphLike G>
GRIPLayoutAlgorithm<G>::GRIPLayoutAlgorithm(G &graph)
    : m_graph(&graph), m_placed(graph.Size(), 0), m_placementSystem(graph),
      m_filtrationOutput(nullptr), m_filtrationSystem(graph) {
  printf("entered grip consturcer\n");
  DimensionResource *dim = m_graph->template GetResource<DimensionResource>();
  if (dim == nullptr)
    throw MissingResourceException<DimensionResource>();
  m_dimension = *dim;

  BFSScratch *scratch = m_graph->template GetResource<BFSScratch>();
  if (scratch == nullptr) {
    printf("SET BFSScratch RESOURCE\n");
    m_graph->template SetResource<BFSScratch>(m_graph->Size());
  }
  printf("AFTER SET BFSScratch RESOURCE\n");

  m_filtrationOutput = m_graph->template GetResource<NestedFiltrationResult>();
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::RunFiltration() {
  m_filtrationSystem.Tick();
  m_currLayer = m_filtrationOutput->m_layerCount - 1;
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::Tick() {
  if (m_filtrationOutput->m_layerCount == 0xFFFFFFFF) {
    // filtration not run
    throw UinitializedResourceException<NestedFiltrationResult>();
  }

  placeLayer();

  // TODO: placement and refinement
}

void MakeRegularSimplex(size_t n, double sideLength, double *out) {
  std::fill(out, out + (n + 1) * n, 0.0);

  for (size_t k = 0; k <= n; k++) {
    double *vk = out + k * n;

    if (k == 0) {
      vk[0] = 1.0;
      continue;
    }

    double c = -1.0 / static_cast<double>(n);

    for (size_t j = 0; j < k; j++) {
      double *vj = out + j * n;
      double dotSoFar = 0.0;
      for (size_t m = 0; m < j; m++)
        dotSoFar += vk[m] * vj[m];
      vk[j] = (c - dotSoFar) / vj[j];
    }

    double normSq = 0.0;
    for (size_t m = 0; m < k; m++)
      normSq += vk[m] * vk[m];
    if (k < n) {
      double rem = 1.0 - normSq;
      vk[k] = rem > 0.0 ? std::sqrt(rem) : 0.0;
    }
  }

  double *v0 = out;
  double *v1 = out + n;
  double unitEdge = 0.0;
  for (size_t m = 0; m < n; m++) {
    double d = v0[m] - v1[m];
    unitEdge += d * d;
  }
  unitEdge = std::sqrt(unitEdge);

  double scale = sideLength / unitEdge;
  for (size_t i = 0; i < (n + 1) * n; i++)
    out[i] *= scale;
}

template <GraphLike G> void GRIPLayoutAlgorithm<G>::placeLayer() {
  if (m_currLayer < 0)
    return;
  auto filtration = m_filtrationOutput;
  if (m_currLayer == filtration->m_layerCount - 1) {
    // TODO: simplex

    std::vector<double> simplex(static_cast<uint8_t>(m_dimension) *
                                (static_cast<uint8_t>(m_dimension) + 1));
    MakeRegularSimplex(static_cast<uint8_t>(m_dimension), 100000.0,
                       simplex.data());

    DenseComponentPool<PositionComponent> &positions =
        m_graph->NodeSpace().template GetPool<PositionComponent>();

    for (uint32_t i = 0; i < filtration->m_borders[m_currLayer]; i++) {
      PositionComponent pos{{}, {simplex[2 * i + 0], simplex[2 * i + 1], 0.0}};
      positions.Set(filtration->m_filtration[i].Raw(), pos);
      m_placed.Set(filtration->m_filtration[i]);
    }

    m_currLayer--;
    return;
  }

  for (uint32_t i = filtration->m_borders[m_currLayer + 1];
       i < filtration->m_borders[m_currLayer]; i++) {
    m_placementSystem.PlaceBetweenKNearest(
        m_graph->MapToSparse(filtration->m_filtration[i]), 10, m_placed);
    m_placed.Set(filtration->m_filtration[i]);
  }
  m_currLayer--;
}
