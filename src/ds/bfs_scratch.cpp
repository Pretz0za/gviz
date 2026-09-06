#include "ds/bfs_scratch.hpp"
#include "graph/search/types.hpp"
#include "graph/types.hpp"

BFSScratch::BFSScratch(size_t size) : m_queue(), m_visited(size, 0) {}

void BFSScratch::InitNew() {
  m_visited.ClearAll();
  m_queue.clear();
}

void BFSScratch::Visit(NodeID node) { m_visited.Set(node.Raw()); }
void BFSScratch::Push(NodeID node, uint32_t depth) {
  m_queue.push_back(FoundNode{node, depth});
}

bool BFSScratch::Empty() const { return m_queue.empty(); }

bool BFSScratch::IsVisited(NodeID node) const {
  return m_visited.Test(node.Raw());
}

FoundNode BFSScratch::Pop() {
  auto out = m_queue.front();
  m_queue.pop_front();
  return out;
}
