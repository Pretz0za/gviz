#pragma once

#include "ds/state_machine.hpp"

template <typename T>
StateMachine<T>::StateMachine(T initial)
    : m_statePool(m_stateSpace.SetPool<DataComponent<T>>()),
      m_current(AddState(initial)) {}

template <typename T> NodeID StateMachine<T>::FindState(T state) const {
  for (NodeID id : m_graph.Nodes())
    if (m_statePool->Find(id.Raw())->data == state)
      return id;
  return INVALID_NODE_ID;
}

template <typename T> bool StateMachine<T>::HasState(T state) const {
  return FindState(state).IsValid();
}

template <typename T> NodeID StateMachine<T>::AddState(T state) {
  NodeID existing = FindState(state);
  if (existing.IsValid())
    return existing;
  NodeID id = m_graph.AddNode();
  m_stateSpace.Create(id.Raw());
  m_statePool->Find(id.Raw())->data = state;
  return id;
}

template <typename T> void StateMachine<T>::AddTransition(T from, T to) {
  m_graph.AddEdge(AddState(from), AddState(to));
}

template <typename T> bool StateMachine<T>::CanTransition(T to) const {
  for (const AdjEntry &entry : m_graph.OutNeighbors(m_current))
    if (m_statePool->Find(entry.other.Raw())->data == to)
      return true;
  return false;
}

template <typename T> bool StateMachine<T>::Transition(T to) {
  for (const AdjEntry &entry : m_graph.OutNeighbors(m_current)) {
    if (m_statePool->Find(entry.other.Raw())->data == to) {
      m_current = entry.other;
      return true;
    }
  }
  return false;
}

template <typename T> std::vector<T> StateMachine<T>::Transitions() const {
  std::vector<T> result;
  result.reserve(m_graph.OutNeighbors(m_current).size());
  for (const AdjEntry &entry : m_graph.OutNeighbors(m_current))
    result.push_back(m_statePool->Find(entry.other.Raw())->data);
  return result;
}

template <typename T> T StateMachine<T>::Current() const {
  return m_statePool->Find(m_current.Raw())->data;
}
