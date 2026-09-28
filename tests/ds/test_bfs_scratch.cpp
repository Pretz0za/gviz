// BFSScratch is a reusable BFS work-list: a FIFO queue of (NodeID, depth)
// pairs plus a DenseNodeID visited-set, meant to be reset with InitNew()
// and reused across many searches instead of reallocating each time.

#include "catch_amalgamated.hpp"
#include "ds/bfs_scratch.hpp"

TEST_CASE("A fresh BFSScratch starts empty and with nothing visited",
         "[ds][bfs_scratch]") {
  BFSScratch scratch(16);
  REQUIRE(scratch.Empty());
  REQUIRE_FALSE(scratch.IsVisited(DenseNodeID(0)));
}

TEST_CASE("Push/Pop behave like a FIFO queue", "[ds][bfs_scratch]") {
  BFSScratch scratch(16);
  scratch.Push(NodeID(1), 0);
  scratch.Push(NodeID(2), 1);
  scratch.Push(NodeID(3), 1);

  REQUIRE_FALSE(scratch.Empty());

  FoundNode first = scratch.Pop();
  REQUIRE(first.node == NodeID(1));
  REQUIRE(first.depth == 0);

  FoundNode second = scratch.Pop();
  REQUIRE(second.node == NodeID(2));
  REQUIRE(second.depth == 1);

  FoundNode third = scratch.Pop();
  REQUIRE(third.node == NodeID(3));
  REQUIRE(third.depth == 1);

  REQUIRE(scratch.Empty());
}

TEST_CASE("Visit marks a node as visited without affecting the queue",
         "[ds][bfs_scratch]") {
  BFSScratch scratch(16);
  scratch.Visit(DenseNodeID(4));

  REQUIRE(scratch.IsVisited(DenseNodeID(4)));
  REQUIRE_FALSE(scratch.IsVisited(DenseNodeID(5)));
  REQUIRE(scratch.Empty());
}

TEST_CASE("InitNew clears both the visited set and the queue",
         "[ds][bfs_scratch]") {
  BFSScratch scratch(16);
  scratch.Push(NodeID(1), 0);
  scratch.Visit(DenseNodeID(1));

  scratch.InitNew();

  REQUIRE(scratch.Empty());
  REQUIRE_FALSE(scratch.IsVisited(DenseNodeID(1)));
}

TEST_CASE("InitNew allows a scratch buffer to be reused for a second search",
         "[ds][bfs_scratch]") {
  BFSScratch scratch(16);
  scratch.Push(NodeID(0), 0);
  scratch.Visit(DenseNodeID(0));
  while (!scratch.Empty())
    scratch.Pop();

  scratch.InitNew();
  scratch.Push(NodeID(5), 0);
  scratch.Visit(DenseNodeID(5));

  REQUIRE(scratch.IsVisited(DenseNodeID(5)));
  REQUIRE_FALSE(scratch.IsVisited(DenseNodeID(0)));
  FoundNode fn = scratch.Pop();
  REQUIRE(fn.node == NodeID(5));
}
