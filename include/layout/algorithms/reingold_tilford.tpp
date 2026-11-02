#pragma once

#include "concept/graphLike.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include "layout/types.hpp"
#include "reingold_tilford.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>

template <GraphLike G>
ReingoldTilfordLayoutAlgorithm<G>::ReingoldTilfordLayoutAlgorithm(
    G &graph, NodeID root, Configuration *config)
    : m_dg(graph), m_view(m_dg, root), m_config(config),
      m_visible(
          &m_dg.template SetResource<VisibleNodesResource>(graph.Size(), 0)),
      m_positions(m_dg.nodeSpace.template SetPool<PositionComponent>(
          config->dimension)),
      m_radii(m_dg.nodeSpace.template SetPool<RadiusComponent>(
          RadiusFunction::CONSTANT)),
      m_colors(m_dg.nodeSpace.template SetPool<ColorComponent>()),
      m_placedRoots{}, m_stack{}, m_depthStack{},
      m_rtDecorators(m_dg.nodeSpace.template SetPool<RTNodeDecorator>()),
      m_stateMachine{States::Initial} {
  m_animation.Init(m_dg);
  m_animFrom = m_dg.nodeSpace.template GetPool<AnimationFromPositionComponent>();
  m_animTo = m_dg.nodeSpace.template GetPool<AnimationToPositionComponent>();
  m_radii->SetConstantRadius(config->radius);

  m_stateMachine.AddTransition(States::Initial, States::Terminal);
  m_stateMachine.AddTransition(States::Initial, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::Comparison);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::Terminal);
  m_stateMachine.AddTransition(States::Comparison, States::Comparison);
  m_stateMachine.AddTransition(States::Comparison, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::Comparison, States::Terminal);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::SetRoot(NodeID id) {
  return m_view.SetRoot(id);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::isRoot(DenseNodeID node) const {
  return m_view.MapToSparse(node) == m_view.Root();
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::finishPlacement(DenseNodeID node) {
  m_colors->Find(node.Raw())->color = kJustPlacedColor;
  m_justPlacedHighlight = node;

  m_lastSubtreePlaced = node;
  m_stateMachine.Transition(isRoot(node) ? States::Terminal
                                         : States::SubtreePlaced);
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::snapToFinalPosition(DenseNodeID node) {
  uint8_t dim = m_config->dimension;
  AnimationFromPositionComponent *from = m_animFrom->Find(node.Raw());
  AnimationToPositionComponent *to = m_animTo->Find(node.Raw());
  PositionComponent *pos = m_positions->Find(node.Raw());
  for (uint8_t d = 0; d < dim; d++) {
    from->pos[d] = to->pos[d];
    pos->pos[d] = to->pos[d];
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::highlightComparison(NodeID a,
                                                            NodeID b) {
  clearComparisonHighlight();
  m_highlightA = a;
  m_highlightB = b;
  m_colors->Find(m_view.MapToDense(a).Raw())->color = kComparingColor;
  m_colors->Find(m_view.MapToDense(b).Raw())->color = kComparingColor;
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::clearComparisonHighlight() {
  if (m_highlightA.IsValid())
    m_colors->Find(m_view.MapToDense(m_highlightA).Raw())->color =
        kPlacedColor;
  if (m_highlightB.IsValid())
    m_colors->Find(m_view.MapToDense(m_highlightB).Raw())->color =
        kPlacedColor;
  m_highlightA = INVALID_NODE_ID;
  m_highlightB = INVALID_NODE_ID;
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::PlaceAll() {
  while (m_stateMachine.Current() != States::Terminal)
    advanceOneStep();
  writePositions();
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::PlaceNext() {
  if (m_stateMachine.Current() == States::Terminal)
    return;

  advanceOneStep();

  m_animation.Begin();
  writePositions();
  m_animation.End(m_config->step_animation_seconds, Easing::EaseInOutCubic);

  if (m_newlyRevealed.IsValid())
    snapToFinalPosition(m_newlyRevealed);
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::Tick() {
  m_animation.Advance();

  uint64_t hash = m_config->Hash();
  if (m_configInitialized && hash == m_lastConfigHash)
    return;
  m_configInitialized = true;
  m_lastConfigHash = hash;

  m_radii->SetConstantRadius(m_config->radius);
  writePositions();
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::advanceOneStep() {
  if (m_justPlacedHighlight.IsValid()) {
    m_colors->Find(m_justPlacedHighlight.Raw())->color = kPlacedColor;
    m_justPlacedHighlight = INVALID_DENSE_NODE_ID;
  }
  m_newlyRevealed = INVALID_DENSE_NODE_ID;

  switch (m_stateMachine.Current()) {
  case States::Initial:
    m_stack.push_back(m_view.Root());
    m_depthStack.push_back(0);
    handleExpand(expand());
    return;

  case States::SubtreePlaced:
    handleExpand(expand());
    return;

  case States::Comparison:
    advanceComparison();
    return;

  case States::Terminal:
    return;
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::handleExpand(DenseNodeID found) {
  if (m_stateMachine.Current() == States::Comparison) {
    beginComparison();
    return;
  }
  removePlacedChildren(found);
  m_placedRoots.push_back(found);
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::removePlacedChildren(
    DenseNodeID found) {
  NodeID sparse = m_view.MapToSparse(found);
  for (const AdjEntry &adj : m_view.Children(sparse)) {
    DenseNodeID child = m_view.MapToDense(adj.other);
    auto it = std::find(m_placedRoots.begin(), m_placedRoots.end(), child);
    if (it != m_placedRoots.end())
      m_placedRoots.erase(it);
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::beginComparison() {
  ComparisonState &s = *m_comparisonState;
  DenseNodeID lcaDense = m_view.MapToDense(s.leastCommonAncestor);
  RTNodeDecorator *lcaDec = m_rtDecorators->Find(lcaDense.Raw());

  DenseNodeID firstChildDense = m_view.MapToDense(s.prevSibling.other);
  RTNodeDecorator *firstChildDec = m_rtDecorators->Find(firstChildDense.Raw());

  lcaDec->offsets.assign(m_view.ChildCount(s.leastCommonAncestor), 0.0);
  lcaDec->lMost = firstChildDec->lMost;
  lcaDec->rMost = firstChildDec->rMost;
  s.defaultAncestor = s.prevSibling.other;

  auto it = std::find(m_placedRoots.begin(), m_placedRoots.end(),
                      firstChildDense);
  assert(it != m_placedRoots.end());
  *it = lcaDense;
  if (it + 1 != m_placedRoots.end())
    m_placedRoots.erase(it + 1);

  updateExtent();
  highlightComparison(s.lrContour, s.rlContour);
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::advanceComparison() {
  ComparisonState &s = *m_comparisonState;
  if (s.currsep < 1.0) {
    comparisonStep();
    return;
  }

  if (iterateContours()) {
    highlightComparison(s.lrContour, s.rlContour);
    return;
  }

  if (comparisonConclude()) {
    highlightComparison(s.lrContour, s.rlContour);
    return;
  }

  DenseNodeID lcaDense = m_view.MapToDense(s.leastCommonAncestor);
  RTNodeDecorator *lcaDec = m_rtDecorators->Find(lcaDense.Raw());
  double center = lcaDec->offsets.back() / 2.0;
  for (double &offset : lcaDec->offsets)
    offset -= center;
  lcaDec->lMax += center;
  lcaDec->rMax -= center;

  m_rtDecorators->Find(m_view.MapToDense(lcaDec->lMost).Raw())->offsets.back() +=
      center;
  if (lcaDec->rMost != lcaDec->lMost)
    m_rtDecorators->Find(m_view.MapToDense(lcaDec->rMost).Raw())
        ->offsets.back() += center;
  for (NodeID abandoned : s.abandonedLMosts)
    m_rtDecorators->Find(m_view.MapToDense(abandoned).Raw())->offsets.back() -=
        center;

  clearComparisonHighlight();
  m_visible->Set(lcaDense);
  finishPlacement(lcaDense);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::iterateContours() {
  ComparisonState &s = *m_comparisonState;

  if (isContourEnd(s.lrContour) || isContourEnd(s.rlContour))
    return false;

  DenseNodeID rlDense = m_view.MapToDense(s.rlContour);
  RTNodeDecorator *rlDec = m_rtDecorators->Find(rlDense.Raw());
  NodeID rlNext = m_view.FirstChild(s.rlContour).other;
  if (!rlNext.IsValid())
    rlNext = rlDec->threadTo;
  double rStep = rlDec->offsets.front();
  s.rOffset += rStep;
  s.currsep += rStep;
  s.rlContour = rlNext;

  DenseNodeID lrDense = m_view.MapToDense(s.lrContour);
  RTNodeDecorator *lrDec = m_rtDecorators->Find(lrDense.Raw());
  NodeID lrNext = lrDec->threadTo;
  if (!lrNext.IsValid())
    lrNext = m_view.LastChild(s.lrContour).other;
  double lStep = lrDec->offsets.back();
  s.lOffset += lStep;
  s.currsep -= lStep;
  s.lrContour = lrNext;

  return true;
}

template <GraphLike G>
NodeID ReingoldTilfordLayoutAlgorithm<G>::getAncestor(NodeID node) {
  DenseNodeID dense = m_view.MapToDense(node);
  NodeID ancestor = m_rtDecorators->Find(dense.Raw())->ancestor;
  AdjEntry adj =
      m_view.FindChild(m_comparisonState->leastCommonAncestor, ancestor);
  if (adj.edge.IsValid() && adj.other.IsValid())
    return ancestor;
  return m_comparisonState->defaultAncestor;
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::markAncestorAlongRightContour(
    NodeID start) {
  NodeID curr = start;

  while (curr.IsValid()) {
    DenseNodeID dense = m_view.MapToDense(curr);
    RTNodeDecorator *dec = m_rtDecorators->Find(dense.Raw());
    dec->ancestor = start;

    NodeID next = dec->threadTo;
    if (!next.IsValid())
      next = m_view.LastChild(curr).other;
    curr = next;
  }
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::comparisonStep() {
  ComparisonState &s = *m_comparisonState;
  NodeID ancestor = getAncestor(s.lrContour);
  NodeID rightSubtree = (*s.it).other;
  uint32_t n = m_view.SiblingAdjacencyDist(s.leastCommonAncestor, ancestor,
                                          rightSubtree);
  double splitSeparation = (1.0 - s.currsep) / static_cast<double>(n);

  std::vector<double> &offsets =
      m_rtDecorators->Find(m_view.MapToDense(s.leastCommonAncestor).Raw())
          ->offsets;

  double newSeparation = 0.0;
  for (uint32_t i = 0; i < n; i++) {
    uint32_t idx = s.currRightSubtreeIdx - i;
    uint32_t mult = n - i;
    double delta = splitSeparation * static_cast<double>(mult);
    offsets[idx] += delta;
    newSeparation += delta;
  }

  s.totalNewSeparation += newSeparation;
  s.currsep = 1.0;

  updateExtent();
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::updateExtent() {
  ComparisonState &s = *m_comparisonState;
  DenseNodeID lcaDense = m_view.MapToDense(s.leastCommonAncestor);
  RTNodeDecorator *lcaDec = m_rtDecorators->Find(lcaDense.Raw());

  DenseNodeID firstChildDense =
      m_view.MapToDense(m_view.FirstChild(s.leastCommonAncestor).other);
  RTNodeDecorator *firstChildDec = m_rtDecorators->Find(firstChildDense.Raw());

  NodeID rightSubtree = (*s.it).other;
  DenseNodeID rightDense = m_view.MapToDense(rightSubtree);
  RTNodeDecorator *rightDec = m_rtDecorators->Find(rightDense.Raw());

  lcaDec->lMax = std::max(firstChildDec->lMax + lcaDec->offsets[0],
                          rightDec->lMax -
                              lcaDec->offsets[s.currRightSubtreeIdx]);
  lcaDec->rMax = std::max(firstChildDec->rMax - lcaDec->offsets[0],
                          rightDec->rMax +
                              lcaDec->offsets[s.currRightSubtreeIdx]);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::isContourEnd(NodeID node) const {
  if (!m_view.IsLeaf(node))
    return false;

  DenseNodeID dense = m_view.MapToDense(node);
  return !m_rtDecorators->Find(dense.Raw())->threadTo.IsValid();
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::comparisonConclude() {
  ComparisonState &s = *m_comparisonState;
  DenseNodeID lcaDense = m_view.MapToDense(s.leastCommonAncestor);
  RTNodeDecorator *lcaDec = m_rtDecorators->Find(lcaDense.Raw());

  NodeID rightSubtree = (*s.it).other;
  DenseNodeID rightDense = m_view.MapToDense(rightSubtree);
  RTNodeDecorator *rightDec = m_rtDecorators->Find(rightDense.Raw());

  if (isContourEnd(s.lrContour) && !isContourEnd(s.rlContour)) {
    s.defaultAncestor = rightSubtree;

    DenseNodeID rlDense = m_view.MapToDense(s.rlContour);
    RTNodeDecorator *rlDec = m_rtDecorators->Find(rlDense.Raw());
    NodeID rlNext = m_view.FirstChild(s.rlContour).other;
    if (!rlNext.IsValid())
      rlNext = rlDec->threadTo;
    double rStep = rlDec->offsets.front();

    s.abandonedLMosts.push_back(lcaDec->lMost);
    RTNodeDecorator *lMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcaDec->lMost).Raw());
    lMostDec->threadTo = rlNext;
    double toOldRootPos = lMostDec->offsets.back();
    lMostDec->offsets.clear();
    lMostDec->offsets.push_back(toOldRootPos + (s.totalNewSeparation / 2.0) +
                                lcaDec->offsets[s.currRightSubtreeIdx] +
                                s.rOffset + rStep);

    lcaDec->lMost = rightDec->lMost;
    lcaDec->rMost = rightDec->rMost;

    RTNodeDecorator *newLMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcaDec->lMost).Raw());
    double lOgOffset = newLMostDec->offsets.back();
    newLMostDec->offsets.clear();
    newLMostDec->offsets.push_back(lOgOffset -
                                   lcaDec->offsets[s.currRightSubtreeIdx]);

    if (lcaDec->rMost != lcaDec->lMost) {
      RTNodeDecorator *newRMostDec =
          m_rtDecorators->Find(m_view.MapToDense(lcaDec->rMost).Raw());
      double rOgOffset = newRMostDec->offsets.back();
      newRMostDec->offsets.clear();
      newRMostDec->offsets.push_back(rOgOffset -
                                     lcaDec->offsets[s.currRightSubtreeIdx]);
    }

  } else if (!isContourEnd(s.lrContour) && isContourEnd(s.rlContour)) {
    markAncestorAlongRightContour(rightSubtree);

    DenseNodeID lrDense = m_view.MapToDense(s.lrContour);
    RTNodeDecorator *lrDec = m_rtDecorators->Find(lrDense.Raw());
    NodeID lrNext = lrDec->threadTo;
    if (!lrNext.IsValid())
      lrNext = m_view.LastChild(s.lrContour).other;
    double lStep = lrDec->offsets.back();

    RTNodeDecorator *rMostDec =
        m_rtDecorators->Find(m_view.MapToDense(rightDec->rMost).Raw());
    rMostDec->threadTo = lrNext;
    double toRightSubtree = rMostDec->offsets.back();
    rMostDec->offsets.clear();
    rMostDec->offsets.push_back(
        toRightSubtree - lcaDec->offsets[s.currRightSubtreeIdx] +
        lcaDec->offsets[s.currRightSubtreeIdx - 1] + s.lOffset + lStep);

    RTNodeDecorator *lMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcaDec->lMost).Raw());
    double ogOffset = lMostDec->offsets.back();
    lMostDec->offsets.clear();
    lMostDec->offsets.push_back(ogOffset + (s.totalNewSeparation / 2.0));

    if (lcaDec->rMost != lcaDec->lMost) {
      RTNodeDecorator *rMostTopDec =
          m_rtDecorators->Find(m_view.MapToDense(lcaDec->rMost).Raw());
      ogOffset = rMostTopDec->offsets.back();
      rMostTopDec->offsets.clear();
      rMostTopDec->offsets.push_back(ogOffset + (s.totalNewSeparation / 2.0));
    }

  } else {
    s.defaultAncestor = rightSubtree;

    RTNodeDecorator *lMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcaDec->lMost).Raw());
    double ogOffset = lMostDec->offsets.back();
    lMostDec->offsets.clear();
    lMostDec->offsets.push_back(ogOffset + (s.totalNewSeparation / 2.0));

    lcaDec->rMost = rightDec->rMost;
    RTNodeDecorator *newRMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcaDec->rMost).Raw());
    ogOffset = newRMostDec->offsets.back();
    newRMostDec->offsets.clear();
    newRMostDec->offsets.push_back(ogOffset -
                                   lcaDec->offsets[s.currRightSubtreeIdx]);
  }

  bool cont = s.iterate();

  if (cont) {
    NodeID newRight = (*s.it).other;
    DenseNodeID denseRight = m_view.MapToDense(newRight);
    auto it =
        std::find(m_placedRoots.begin(), m_placedRoots.end(), denseRight);
    assert(it != m_placedRoots.end());
    m_placedRoots.erase(it);

    lcaDec->offsets[s.currRightSubtreeIdx] =
        lcaDec->offsets[s.currRightSubtreeIdx - 1];
    updateExtent();
  }

  return cont;
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::solveLeafDecorators(DenseNodeID node,
                                                            uint32_t depth) {
  NodeID sparse = m_view.MapToSparse(node);
  RTNodeDecorator *dec = m_rtDecorators->Find(node.Raw());
  dec->lMost = sparse;
  dec->rMost = sparse;
  dec->ancestor = sparse;
  dec->depth = depth;
  dec->threadTo = INVALID_NODE_ID;
  dec->lMax = 0.0;
  dec->rMax = 0.0;
  dec->offsets.assign(1, 0.0);
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::solveSingleChildDecorators(
    DenseNodeID node, uint32_t depth) {
  NodeID sparse = m_view.MapToSparse(node);
  NodeID child = m_view.FirstChild(sparse).other;
  DenseNodeID childDense = m_view.MapToDense(child);
  RTNodeDecorator *childDec = m_rtDecorators->Find(childDense.Raw());

  RTNodeDecorator *dec = m_rtDecorators->Find(node.Raw());
  dec->offsets.assign(1, 0.0);
  dec->lMost = childDec->lMost;
  dec->rMost = childDec->rMost;
  dec->lMax = childDec->lMax;
  dec->rMax = childDec->rMax;
  dec->depth = depth;

  markAncestorAlongRightContour(sparse);
}

template <GraphLike G> DenseNodeID ReingoldTilfordLayoutAlgorithm<G>::expand() {
  NodeID target = m_stack.back();
  uint32_t depth = m_depthStack.back();
  AdjEntry probe = m_view.FirstChild(target);

  if (!probe.edge.IsValid() || !probe.other.IsValid()) {
    DenseNodeID dense = m_view.MapToDense(target);
    m_stack.pop_back();
    m_depthStack.pop_back();

    m_visible->Set(dense);
    solveLeafDecorators(dense, depth);
    m_newlyRevealed = dense;
    finishPlacement(dense);
    return dense;
  }

  DenseNodeID dProbe = m_view.MapToDense(probe.other);
  if (m_visible->Test(dProbe)) {
    if (dProbe == m_lastSubtreePlaced) {
      DenseNodeID dense = m_view.MapToDense(target);
      m_stack.pop_back();
      m_depthStack.pop_back();

      solveSingleChildDecorators(dense, depth);
      m_visible->Set(dense);
      m_newlyRevealed = dense;
      finishPlacement(dense);
      return dense;
    }

    DenseNodeID dense = m_view.MapToDense(target);
    m_stack.pop_back();
    m_depthStack.pop_back();
    m_rtDecorators->Find(dense.Raw())->depth = depth;

    m_stateMachine.Transition(States::Comparison);
    m_comparisonState.emplace(m_view.Children(target));
    m_comparisonState->leastCommonAncestor = target;
    m_comparisonState->iterate();
    return dense;
  }

  size_t l = m_stack.size();
  for (const AdjEntry &adj : m_view.Children(target))
    m_stack.push_back(adj.other);
  std::reverse(m_stack.begin() + l, m_stack.end());
  m_depthStack.insert(m_depthStack.end(), m_stack.size() - l, depth + 1);

  return expand();
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::writePositions() {
  double x = 0.0;
  double prevRMax = 0.0;

  for (size_t i = 0; i < m_placedRoots.size(); i++) {
    DenseNodeID rootDense = m_placedRoots[i];
    RTNodeDecorator *dec = m_rtDecorators->Find(rootDense.Raw());

    if (i > 0)
      x += (prevRMax + dec->lMax) * m_config->min_separation +
           2.0 * m_config->radius;

    NodeID skipChild = (i + 1 < m_placedRoots.size())
                           ? m_view.MapToSparse(m_placedRoots[i + 1])
                           : INVALID_NODE_ID;

    placePreorder(m_view.MapToSparse(rootDense), x, skipChild);

    prevRMax = dec->rMax;
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::writeAxes(PositionComponent *pos,
                                                  double spread,
                                                  double depthValue) const {
  switch (m_config->direction) {
  case Configuration::Direction::Down:
    pos->pos[0] = spread;
    pos->pos[1] = depthValue;
    return;
  case Configuration::Direction::Up:
    pos->pos[0] = spread;
    pos->pos[1] = -depthValue;
    return;
  case Configuration::Direction::Left:
    pos->pos[0] = -depthValue;
    pos->pos[1] = spread;
    return;
  case Configuration::Direction::Right:
    pos->pos[0] = depthValue;
    pos->pos[1] = spread;
    return;
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::placePreorder(NodeID node, double x,
                                                      NodeID skipChild) {
  DenseNodeID dense = m_view.MapToDense(node);
  RTNodeDecorator *dec = m_rtDecorators->Find(dense.Raw());

  PositionComponent *pos = m_positions->Find(dense.Raw());
  writeAxes(pos, x, static_cast<double>(dec->depth) * m_config->layer_separation);

  uint32_t idx = 0;
  for (const AdjEntry &adj : m_view.Children(node)) {
    if (adj.other == skipChild)
      break;
    double childX = x + dec->offsets[idx] * m_config->min_separation;
    placePreorder(adj.other, childX, INVALID_NODE_ID);
    idx++;
  }
}
