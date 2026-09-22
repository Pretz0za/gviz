#pragma once

#include "ds/quadtree.hpp"
#include "graph/types.hpp"
#include <memory>

struct QuadTreeResource {
  std::unique_ptr<SpacialIndex::QuadTree<DenseNodeID, 1>> root;
};
