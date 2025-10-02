#include <gtest/gtest.h>
#include <vector>
#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"

using namespace bmssp;

TEST(BMSSPDriver, SimpleLine) {
  // Create a simple line graph: 0 -> 1 -> 2 -> 3
  Graph g(4);
  g.add_edge(0, 1, 10);
  g.add_edge(1, 2, 20);
  g.add_edge(2, 3, 30);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 10u);
  EXPECT_EQ(dist[2], 30u);
  EXPECT_EQ(dist[3], 60u);
}

TEST(BMSSPDriver, SmallGraph) {
  // Diamond graph with multiple paths
  Graph g(5);
  g.add_edge(0, 1, 1);
  g.add_edge(0, 2, 4);
  g.add_edge(1, 2, 2);
  g.add_edge(1, 3, 5);
  g.add_edge(2, 3, 1);
  g.add_edge(2, 4, 3);
  g.add_edge(3, 4, 2);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 1u);
  EXPECT_EQ(dist[2], 3u);
  EXPECT_EQ(dist[3], 4u);
  EXPECT_EQ(dist[4], 6u);
}
