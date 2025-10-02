#include <gtest/gtest.h>
#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"

using namespace bmssp;

TEST(EqualDistance, TieBreakingByHopCount) {
  // Diamond graph: 0 has two paths to 3
  //    0
  //   / \
  //  1   2  (both edges weight 1)
  //   \ /
  //    3
  // Two equal-length paths to vertex 3
  Graph g(4);
  g.add_edge(0, 1, 1);
  g.add_edge(0, 2, 1);
  g.add_edge(1, 3, 1);
  g.add_edge(2, 3, 1);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 1u);
  EXPECT_EQ(dist[2], 1u);
  EXPECT_EQ(dist[3], 2u);  // Both paths have length 2
}

TEST(EqualDistance, MultiplePathsSameWeight) {
  // Graph with multiple equal-length paths
  //      0
  //    / | \
  //   1  2  3  (all weight 5)
  //    \ | /
  //      4     (all weight 5)
  Graph g(5);
  g.add_edge(0, 1, 5);
  g.add_edge(0, 2, 5);
  g.add_edge(0, 3, 5);
  g.add_edge(1, 4, 5);
  g.add_edge(2, 4, 5);
  g.add_edge(3, 4, 5);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 5u);
  EXPECT_EQ(dist[2], 5u);
  EXPECT_EQ(dist[3], 5u);
  EXPECT_EQ(dist[4], 10u);  // All three paths have same length
}

TEST(EqualDistance, ComplexTieBreaking) {
  // More complex graph to test tie-breaking
  //   0 --1--> 1
  //   |        |
  //   1        1
  //   |        |
  //   v        v
  //   2 --1--> 3
  // Both paths 0->1->3 and 0->2->3 have distance 2
  Graph g(4);
  g.add_edge(0, 1, 1);
  g.add_edge(0, 2, 1);
  g.add_edge(1, 3, 1);
  g.add_edge(2, 3, 1);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 1u);
  EXPECT_EQ(dist[2], 1u);
  EXPECT_EQ(dist[3], 2u);  // Two equal paths
}

TEST(EqualDistance, LexicographicOrdering) {
  // Test that when distances and hops are equal, vertex ID breaks ties
  //      0
  //     /|\
  //   1  2  3  (all weight 10)
  //     \|/
  //      4     (all weight 10)
  // Vertex 4 reachable from 1, 2, 3 with same dist and hops
  Graph g(5);
  g.add_edge(0, 1, 10);
  g.add_edge(0, 2, 10);
  g.add_edge(0, 3, 10);
  g.add_edge(1, 4, 10);
  g.add_edge(2, 4, 10);
  g.add_edge(3, 4, 10);
  
  auto dist = run_sssp(g, 0);
  
  // All paths to 4 have same distance (20) and hops (2)
  EXPECT_EQ(dist[4], 20u);
  
  // Run multiple times to ensure determinism
  for (int i = 0; i < 3; ++i) {
    auto dist2 = run_sssp(g, 0);
    EXPECT_EQ(dist[4], dist2[4]) << "Results should be deterministic";
  }
}

TEST(EqualDistance, ZeroWeightEdges) {
  // Graph with zero-weight edges
  //  0 --0--> 1 --0--> 2 --0--> 3
  Graph g(4);
  g.add_edge(0, 1, 0);
  g.add_edge(1, 2, 0);
  g.add_edge(2, 3, 0);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 0u);
  EXPECT_EQ(dist[2], 0u);
  EXPECT_EQ(dist[3], 0u);
}
