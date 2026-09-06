#pragma once

#include "ds/bitset.hpp"
#include "graph/search/types.hpp"
#include "graph/types.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>

class BFSScratch {
public:
  BFSScratch(size_t size);

  void InitNew();

  void Visit(NodeID node);
  void Push(NodeID node, uint32_t depth);
  FoundNode Pop();

  bool Empty() const;
  bool IsVisited(NodeID node) const;

private:
  BitSet m_visited;
  std::deque<FoundNode> m_queue;
};
