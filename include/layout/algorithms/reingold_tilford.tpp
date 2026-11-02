#pragma once

#include "concept/graphLike.hpp"
#include "graph/components/adjacency.hpp"
#include "graph/types.hpp"
#include "layout/components/rt_node_decorator.hpp"
#include "layout/types.hpp"
#include "reingold_tilford.hpp"
#include <algorithm>
#include <cstdint>

template <GraphLike G>
ReingoldTilfordLayoutAlgorithm<G>::ReingoldTilfordLayoutAlgorithm(
    G &graph, NodeID root, Configuration *config)
    : m_dg(graph), m_view(m_dg, root), m_config(config),
      m_visible(
          &m_dg.template SetResource<VisibleNodesResource>(graph.size(), 0)),
      m_placedRoots{}, m_stack{},
      m_rtDecorators(m_dg.nodeSpace.template SetPool<RTNodeDecorator>()),
      m_stateMachine{States::Initial} {
  m_stateMachine.AddTransition(States::Initial, States::Terminal); // empty tree
  m_stateMachine.AddTransition(States::Initial, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::Comparison);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::SubtreePlaced);
  m_stateMachine.AddTransition(States::Comparison, States::Comparison);
  m_stateMachine.AddTransition(States::SubtreePlaced, States::Terminal);
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::SetRoot(NodeID id) {
  return m_view.SetRoot(id);
}

template <GraphLike G> void ReingoldTilfordLayoutAlgorithm<G>::PlaceNext() {
  switch (m_stateMachine.Current()) {
  case States::Initial: {
    m_stack.push_back(m_view.Root());
    DenseNodeID leaf = expand();
    m_placedRoots->push_back(leaf);

    // TODO: animation and aabb stuff
    return;
  }

  case States::SubtreePlaced: {
    if (m_view.MapToSparse(m_lastSubtreePlaced) == m_view.Root()) {
      m_stateMachine.Transition(States::Terminal);
      return;
    }

    DenseNodeID found = expand();
    if (m_stateMachine.Current() == States::Comparison) {
      // place vertices on top of each other and make sure comparison state is
      // set up
      return;
    }
    m_placedRoots->push_back(found);

    // TODO: animation and aabb stuff
    return;
  }

  case States::Comparison: {

    if (m_comparisonState.currsep < 1.0) {
      comparisonStep();
      return;
    }

    bool cont = iterateContours();
    if (!cont)
      comparisonConclude();

    return;
  }

  case States::Terminal:
    return;
  }
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::iterateContours() {
  ComparisonState &s = m_comparisonState;
  NodeID &lr = s.lrContour;
  NodeID &rl = s.rlContour;

  if (isContourEnd(lr) || isContourEnd(rl))
    return false;

  // iterate right subtree left contour
  DenseNodeID rlDense = m_view.MapToDense(rl);
  NodeID rlNext = m_view.FirstChild(rl).other;
  if (!rlNext.IsValid()) {
    rlNext = m_rtDecorators->Find(rlDense.Raw())->threadTo;
  }
  double rStep = m_rtDecorators->Find(rlDense.Raw())->offsets.front();
  s.lOffset += rStep;
  s.currsep += rStep;
  s.rlContour = rlNext;

  // iterate left subtree right contour
  DenseNodeID lrDense = m_view.MapToDense(lr);
  NodeID lrNext = m_rtDecorators->Find(lrDense.Raw())->threadTo;
  if (!lrNext.IsValid()) {
    lrNext = m_view.LastChild(lr).other;
  }
  double lStep = m_rtDecorators->Find(lrDense.Raw())->offsets.back();
  s.rOffset += lStep;
  s.currsep -= lStep;
  s.lrContour = lrNext;

  return true;
}

template <GraphLike G>
NodeID ReingoldTilfordLayoutAlgorithm<G>::getAncestor(NodeID node) {
  DenseNodeID dense = m_view.MapToDense(node);
  NodeID ancestor = m_rtDecorators->Find(dense.Raw())->ancestor;
  AdjEntry adj =
      m_view.FindChild(m_comparisonState.leastCommonAncestor, ancestor);
  if (adj.edge.IsValid() && adj.other.IsValid()) {
    return ancestor;
  }
  return m_comparisonState.defualtAncestor;
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::markAncestorAlongRightContour(
    NodeID start) {

  NodeID curr = start;

  while (curr.IsValid()) {
    DenseNodeID dense = m_view.MapToDense(curr);
    m_rtDecorators->Find(dense.Raw())->ancestor = start;
    curr = m_rtDecorators->Find(dense.Raw())->threadTo;
    if (!curr.IsValid()) {
      curr = m_view.LastChild(dense).other;
    }
  }
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::comparisonConclude() {
  ComparisonState &s = m_comparisonState;
  if (isContourEnd(s.lrContour) && !isContourEnd(s.rlContour)) {
    // ancestor stuff
    s.defualtAncestor = s.currRightSubtreeIdx;

    DenseNodeID lcmDense = m_view.MapToDense(s.leastCommonAncestor);
    RTNodeDecorator *lcmDec = m_rtDecorators->Find(lcmDense.Raw());

    // threading
    DenseNodeID rlDense = m_view.MapToDense(s.rlContour);
    NodeID rlNext = m_view.FirstChild(s.rlContour).other;
    if (!rlNext.IsValid()) {
      rlNext = m_rtDecorators->Find(rlDense.Raw())->threadTo;
    }
    double rStep = m_rtDecorators->Find(rlDense.Raw())->offsets.front();

    RTNodeDecorator *lmostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->lMost));

    lmostDec->threadTo = rlNext;
    double toOldRootPos = lmostDec->offsets.back();
    lmostDec->offsets.clear();

    //      Vector from LMost to the LCM's position pre mering the new child
    // +    Vector from LCM old to new position (totalNewSeparation / 2)
    // +    Vector from LCM new position to just merged child (rightSubtreeIdx)
    // +    Vector from just merged child to where we thread to (offset+step)
    lmostDec->offsets.push_back(toOldRootPos + (s.totalNewSeparation / 2.0) +
                                lcmDec->offsets[s.currRightSubtreeIdx] +
                                s.rOffset + rStep);

    // extremes
    AdjEntry adj = *s.it;
    NodeID rightSubtree = adj.other;
    DenseNodeID rightDense = m_view.MapToDense(rightSubtree);
    RTNodeDecorator *rightDec = m_rtDecorators->Find(rightDense.Raw());

    lcmDec->lMost = rightDec->lMost;
    RTNodeDecorator *newLMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->lMost.Raw()));
    double ogOffset = newLMostDec->offsets.back();
    newLMostDec->offsets.clear();
    newLMostDec->offsets.push_back(ogOffset -
                                   lcmDec->offsets[s.currRightSubtreeIdx]);

    lcmDec->rMost = rightDec->rMost;
    RTNodeDecorator *newRMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->rMost).Raw());
    ogOffset = newRMostDec->offsets.back();
    newRMostDec->offsets.clear();
    newRMostDec->offsets.push_back(ogOffset -
                                   lcmDec->offsets[s.currRightSubtreeIdx]);

  } else if (!isContourEnd(s.lrContour) && isContourEnd(s.rlContour)) {
    // ancestor stuff
    NodeID rightSubtree = (*s.it).other;
    DenseNodeID rightDense = m_view.MapToDense(rightSubtree);
    markAncestorAlongRightContour(rightSubtree);

    DenseNodeID lcmDense = m_view.MapToDense(s.leastCommonAncestor);
    RTNodeDecorator *lcmDec = m_rtDecorators->Find(lcmDense.Raw());

    // threading
    DenseNodeID lrDense = m_view.MapToDense(s.lrContour);
    NodeID lrNext = m_rtDecorators->Find(lrDense.Raw())->threadTo;
    if (!lrNext.IsValid()) {
      lrNext = m_view.LastChild(s.lrContour).other;
    }
    double lStep = m_rtDecorators->Find(lrDense.Raw())->offsets.back();

    RTNodeDecorator *rmostDec = m_rtDecorators->Find(
        m_view.MapToDense(m_rtDecorators->Find(rightDense.Raw())->rMost));

    rmostDec->threadTo = lrNext;
    double toRightSubtree = rmostDec->offsets.back();
    rmostDec->offsets.clear();

    //      Vector from Rmost to right subtree root
    // +    Vector from right subtreee root to its parent' new position (LCM)
    // +    Vector from LCM new position to the child where lrContour started
    // +    Vector from that subtree's root to where we are on the contour lNext
    rmostDec->offsets.push_back(
        toRightSubtree - lcmDec->offsets[s.currRightSubtreeIdx] +
        lcmDec->offsets[s.currRightSubtreeIdx - 1] + s.lOffset + lStep);

    // extremes
    RTNodeDecorator *lMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->lMost.Raw()));
    double ogOffset = lMostDec->offsets.back();
    lMostDec->offsets.clear();
    lMostDec->offsets.push_back(ogOffset + (s.totalNewSeparation / 2.0));

    RTNodeDecorator *rMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->rMost.Raw()));
    ogOffset = rMostDec->offsets.back();
    rMostDec->offsets.clear();
    rMostDec->offsets.push_back(ogOffset + (s.totalNewSeparation / 2.0));

  } else {
    // ancestor stuff
    NodeID rightSubtree = (*s.it).other;
    DenseNodeID rightDense = m_view.MapToDense(rightSubtree);
    RTNodeDecorator *rightDec = m_rtDecorators->Find(rightDense.Raw());
    markAncestorAlongRightContour(rightSubtree);

    DenseNodeID lcmDense = m_view.MapToDense(s.leastCommonAncestor);
    RTNodeDecorator *lcmDec = m_rtDecorators->Find(lcmDense.Raw());

    // extremes
    RTNodeDecorator *lMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->lMost.Raw()));
    double ogOffset = lMostDec->offsets.back();
    lMostDec->offsets.clear();
    lMostDec->offsets.push_back(ogOffset + (s.totalNewSeparation / 2.0));

    lcmDec->rMost = rightDec->rMost;
    RTNodeDecorator *newRMostDec =
        m_rtDecorators->Find(m_view.MapToDense(lcmDec->rMost.Raw()));
    ogOffset = newRMostDec->offsets.back();
    newRMostDec->offsets.clear();
    newRMostDec->offsets.push_back(ogOffset -
                                   lcmDec->offsets[s.currRightSubtreeIdx]);
  }

  DenseNodeID denseParent = m_view.MapToDense(s.leastCommonAncestor);
  DenseNodeID denseRightSubtree = m_view.MapToDense(s.currRightSubtreeIdx);
  DenseNodeID denseFirstChild =
      m_view.MapToDense(m_view.FirstChild(s.leastCommonAncestor));

  RTNodeDecorator *parentDec = m_rtDecorators->Find(denseParent.Raw());
  RTNodeDecorator *rightSubtreeDec =
      m_rtDecorators->Find(denseRightSubtree.Raw());
  RTNodeDecorator *firstChildDec = m_rtDecorators->Find(denseFirstChild.Raw());

  parentDec->lMax = std::max(firstChildDec->lMax + parentDec->offsets[0],
                             rightSubtreeDec->lMax -
                                 parentDec->offsets[s.currRightSubtreeIdx]);

  parentDec->rMax = std::max(firstChildDec->rMax - parentDec->offsets[0],
                             rightSubtreeDec->rMax +
                                 parentDec->offsets[s.currRightSubtreeIdx]);

  return s.iterate();
}

template <GraphLike G>
void ReingoldTilfordLayoutAlgorithm<G>::comparisonStep() {
  auto &s = m_comparisonState;
  if (s.currsep < 1.0) {
    NodeID ancestor = getAncestor(s.lrContour);
    uint32_t n = m_view.SiblingAdjacencyDist(s.leastCommonAncestor, ancestor,
                                             s.currRightSubtreeIdx);
    double splitSeparation = (1.0 - s.currsep) / static_cast<double>(n);
    double newSeparation = 0.0;
    std::vector<double> &offsets =
        m_rtDecorators->Find(m_view.MapToDense(s.leastCommonAncestor).Raw())
            ->offsets;

    for (uint32_t i = 0; i < n; i++) {
      uint32_t idx = s.currRightSubtreeIdx - i;
      uint32_t mult = n - i;
      offsets[idx] += (splitSeparation * static_cast<double>(mult));
      newSeparation += (splitSeparation * static_cast<double>(mult));
    }

    s.totalNewSeparation += newSeparation;
  }
}

template <GraphLike G>
bool ReingoldTilfordLayoutAlgorithm<G>::isContourEnd(NodeID node) const {
  if (!m_view.IsLeaf(node))
    return false;

  DenseNodeID dense = m_view.MapToDense(node);
  return !m_rtDecorators->Find(dense.Raw())->threadTo.IsValid();
}

template <GraphLike G> DenseNodeID ReingoldTilfordLayoutAlgorithm<G>::expand() {
  NodeID target = m_stack.back();
  AdjEntry probe = m_view.FirstChild(target);

  // Base Case 1: leaf node
  if (!probe.edge.IsValid() || !probe.other.IsValid()) {
    DenseNodeID dense = m_view.MapToDense(target);
    m_visible->Set(dense);
    m_stack.pop_back();

    solveLeafDecorators(dense);

    m_stateMachine.Transition(States::SubtreePlaced);
    m_lastSubtreePlaced = dense;
    return dense;
  }

  DenseNodeID dProbe = m_view.MapToDense(probe.other);
  if (m_visible->Test(dProbe)) {
    // Base Case 2: children placed, merge and mark parent placed
    if (dProbe == m_lastSubtreePlaced) {
      DenseNodeID dense = m_view.MapToDense(target);
      solveSingleChildDecorators(dense);
      m_visible->Set(dense);
      m_stack.pop_back();
      m_stateMachine.Transition(States::SubtreePlaced);
      m_lastSubtreePlaced = dense;
      return dense;
    } else {
      m_stateMachine.Transition(States::Comparison);
      m_stack.pop_back();
      m_comparisonState.leastCommonAncestor = target;
      m_comparisonState.generator = m_view.Children(target);
      m_comparisonState.it = m_comparisonState.generator.begin();
      m_comparisonState.currRightSubtreeIdx = 0;
      m_comparisonState.iterate();
      return m_view.MapToDense(target);
    }
  } else {
    size_t l = m_stack.size();
    for (const AdjEntry &adj : m_view.Children(target)) {
      m_stack.push_back(adj.other);
    }

    // reverse m_stack between [l, r) where r = size()
    std::reverse(m_stack.begin() + l, m_stack.end());
    expand();
  }
}
