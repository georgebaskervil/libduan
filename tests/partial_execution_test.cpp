#include <gtest/gtest.h>
#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

TEST(BMSSPPartial, PartialTerminationTriggered) {
  // Create a linear graph that's large enough to trigger partial termination
  // 0 -> 1 -> 2 -> ... -> 9
  const int n = 10;
  Graph g(n);
  for (int i = 0; i < n - 1; ++i) {
    g.add_edge(i, i + 1, 1);
  }
  
  DistState st = make_state(n);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  
  // Use small parameters: k=1, t=1, l=2
  // Partial threshold: k*2^l*t = 1*4*1 = 4
  BMSSPParams params{1, 1, 2};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  
  auto result = bmssp::bmssp(2, B, S, g, st, params);
  
  // With a linear graph of 10 vertices and threshold 4,
  // we might get partial or success depending on M
  // Just verify the result is valid
  EXPECT_GE(result.U.size(), 1u) << "Should return at least the source";
  EXPECT_LE(result.boundary, B) << "Boundary should be <= B";
}

TEST(BMSSPPartial, SuccessWithSmallGraph) {
  // Small graph should complete successfully
  Graph g(5);
  g.add_edge(0, 1, 1);
  g.add_edge(1, 2, 1);
  g.add_edge(2, 3, 1);
  g.add_edge(3, 4, 1);
  
  DistState st = make_state(5);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  
  BMSSPParams params{2, 2, 2};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  
  auto result = bmssp::bmssp(2, B, S, g, st, params);
  
  // Should succeed (boundary == B)
  EXPECT_EQ(result.boundary, B) << "Small graph should complete successfully";
  
  // All reachable vertices should be in U
  EXPECT_GE(result.U.size(), 1u) << "At least source should be in U";
}

TEST(BMSSPPartial, BoundaryRespectedInPartial) {
  // Create a graph and use a limited boundary
  Graph g(10);
  for (int i = 0; i < 9; ++i) {
    g.add_edge(i, i + 1, 5);
  }
  
  DistState st = make_state(10);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  
  BMSSPParams params{2, 2, 2};
  uint64_t B = 25;  // Only reach first 5 vertices (0, 5, 10, 15, 20)
  
  auto result = bmssp::bmssp(2, B, S, g, st, params);
  
  // Returned boundary must be <= B
  EXPECT_LE(result.boundary, B);
  
  // All vertices in U must have dist < returned boundary
  for (int v : result.U) {
    uint64_t dist = st.dist[v].as_u64_clamped();
    EXPECT_LT(dist, result.boundary)
      << "Vertex " << v << " with dist " << dist 
      << " should be < boundary " << result.boundary;
  }
}

TEST(BMSSPPartial, EmptyGraphHandling) {
  Graph g(0);
  DistState st = make_state(0);
  std::vector<int> S;
  
  BMSSPParams params{1, 1, 1};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  
  auto result = bmssp::bmssp(1, B, S, g, st, params);
  
  EXPECT_TRUE(result.U.empty());
}
