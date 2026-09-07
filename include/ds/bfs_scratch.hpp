#pragma once

#include "graph/search/types.hpp"
#include "graph/types.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>

class BFSScratch {
public:
  BFSScratch(size_t size);

  void InitNew();

  void Visit(DenseNodeID node);
  void Push(NodeID node, uint32_t depth);
  FoundNode Pop();

  bool Empty() const;
  bool IsVisited(DenseNodeID node) const;

private:
  DenseNodeSet m_visited;
  std::deque<FoundNode> m_queue;
};
