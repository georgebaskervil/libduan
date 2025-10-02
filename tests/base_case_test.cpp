#include <gtest/gtest.h>
#include "cpp_starter/bmssp/base_case.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

TEST(BaseCase, ReturnsAtMostKVertices) {
  // Create a linear graph: 0 -> 1 -> 2 -> 3 -> 4 -> 5
  Graph g(6);
  for (int i = 0; i < 5; ++i) {
    g.add_edge(i, i + 1, 1);
  }
  
  DistState st = make_state(6);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  mark_complete(st, 0);
  
  // Test with k=3: should return at most 3 vertices
  BaseCaseParams params{3};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = base_case(B, S, g, st, params);
  
  // When there are > k reachable vertices, should return at most k
  EXPECT_LE(result.U.size(), params.k);
  
  // All returned vertices should be complete
  for (int v : result.U) {
    EXPECT_TRUE(st.complete[v]) << "Vertex " << v << " should be marked complete";
  }
}

TEST(BaseCase, ReturnsAllWhenFewerThanK) {
  // Create a small graph with only 2 reachable vertices
  Graph g(5);
  g.add_edge(0, 1, 1);
  // Vertices 2, 3, 4 are unreachable
  
  DistState st = make_state(5);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  mark_complete(st, 0);
  
  // k=5, but only 2 vertices reachable
  BaseCaseParams params{5};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = base_case(B, S, g, st, params);
  
  // Should return all reachable vertices (≤ k)
  EXPECT_LE(result.U.size(), params.k);
  EXPECT_EQ(result.boundary, B) << "Boundary should be B when returning ≤ k vertices";
  
  // Verify source is in result
  EXPECT_NE(std::find(result.U.begin(), result.U.end(), 0), result.U.end());
}

TEST(BaseCase, KPlusOneThreshold) {
  // Create a graph where we can reach exactly k+1 vertices
  // 0 -> 1, 2, 3, 4 (4 neighbors)
  Graph g(5);
  for (int i = 1; i <= 4; ++i) {
    g.add_edge(0, i, 1);
  }
  
  DistState st = make_state(5);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  mark_complete(st, 0);
  
  // k=3, so k+1=4, and we have exactly 5 reachable vertices (source + 4 neighbors)
  BaseCaseParams params{3};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = base_case(B, S, g, st, params);
  
  // When we collect k+1 vertices, boundary should be < B (partial result)
  if (result.U.size() > params.k) {
    EXPECT_LT(result.boundary, B) << "Boundary should be < B for partial result";
    
    // All vertices in U should have dist < boundary
    for (int v : result.U) {
      EXPECT_LT(st.dist[v].as_u64_clamped(), result.boundary)
        << "Vertex " << v << " dist should be < boundary";
    }
  }
}

TEST(BaseCase, BoundaryRespected) {
  // Create a linear graph with increasing weights
  Graph g(5);
  g.add_edge(0, 1, 5);
  g.add_edge(1, 2, 10);
  g.add_edge(2, 3, 15);
  g.add_edge(3, 4, 20);
  
  DistState st = make_state(5);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  mark_complete(st, 0);
  
  BaseCaseParams params{10};
  uint64_t B = 20;  // Boundary at 20, so we can reach: 0(0), 1(5), 2(15) but not 3(30)
  auto result = base_case(B, S, g, st, params);
  
  // All returned vertices must have dist < B
  for (int v : result.U) {
    EXPECT_LT(st.dist[v].as_u64_clamped(), B)
      << "Vertex " << v << " has dist >= boundary";
  }
}

TEST(BaseCase, SingleVertexGraph) {
  Graph g(1);
  
  DistState st = make_state(1);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  mark_complete(st, 0);
  
  BaseCaseParams params{5};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = base_case(B, S, g, st, params);
  
  // Should return just the source
  EXPECT_EQ(result.U.size(), 1u);
  EXPECT_EQ(result.U[0], 0);
  EXPECT_EQ(result.boundary, B);
}
