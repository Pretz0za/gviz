#pragma once

#include "ecs/components.hpp"
#include "graph/components/data.hpp"
#include "graph/graph.hpp"
#include "graph/types.hpp"
#include <vector>

template <typename T> class StateMachine {
public:
  explicit StateMachine(T initial);

  NodeID AddState(T state);
  NodeID FindState(T state) const;
  bool HasState(T state) const;

  void AddTransition(T from, T to);
  bool CanTransition(T to) const;
  bool Transition(T to);

  std::vector<T> Transitions() const;

  T Current() const;

private:
  Graph m_graph;
  IndexSpace m_stateSpace;
  DenseComponentPool<DataComponent<T>> *m_statePool;
  NodeID m_current;
};

#include "ds/state_machine.tpp"
