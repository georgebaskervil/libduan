#include <gtest/gtest.h>
#include "cpp_starter/bmssp/find_pivots.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

TEST(FindPivots, SizeBounds) {
  // Create a simple graph: 0 -> 1 -> 2 -> 3 -> 4
  Graph g(5);
  g.add_edge(0, 1, 1);
  g.add_edge(1, 2, 1);
  g.add_edge(2, 3, 1);
  g.add_edge(3, 4, 1);
  
  DistState st = make_state(5);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  
  // Test with k=2
  FindPivotsParams params{2};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = find_pivots(B, S, g, st, params);
  
  // Verify size bounds
  EXPECT_LE(result.P.size(), (result.W.size() + params.k - 1) / params.k) 
    << "|P| must be ≤ ⌈|W|/k⌉";
  
  // Without early exit, |W| ≤ k|S|
  if (result.W.size() <= params.k * S.size()) {
    // Normal path
    EXPECT_LE(result.W.size(), params.k * S.size())
      << "|W| must be ≤ k|S| when no early exit";
  } else {
    // Early exit path
    EXPECT_EQ(result.P, S) << "Early exit should return P = S";
  }
  
  // P must be subset of S
  for (int p : result.P) {
    EXPECT_NE(std::find(S.begin(), S.end(), p), S.end())
      << "P must be subset of S";
  }
}

TEST(FindPivots, EarlyExitTriggered) {
  // Create a graph with high out-degree to trigger early exit
  // 0 connects to vertices 1-10
  Graph g(11);
  for (int i = 1; i <= 10; ++i) {
    g.add_edge(0, i, 1);
  }
  
  DistState st = make_state(11);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  
  // Use k=2, so k|S| = 2, but W will grow to > 2 (should be ~11)
  FindPivotsParams params{2};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = find_pivots(B, S, g, st, params);
  
  // Early exit should trigger: |W| > k|S|
  EXPECT_GT(result.W.size(), params.k * S.size())
    << "Early exit triggered, so |W| > k|S|";
  
  // P should equal S on early exit
  EXPECT_EQ(result.P.size(), S.size());
  EXPECT_EQ(result.P, S);
}

TEST(FindPivots, EmptySource) {
  Graph g(5);
  g.add_edge(0, 1, 1);
  
  DistState st = make_state(5);
  std::vector<int> S;  // empty
  
  FindPivotsParams params{2};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = find_pivots(B, S, g, st, params);
  
  EXPECT_TRUE(result.P.empty());
  EXPECT_TRUE(result.W.empty());
}

TEST(FindPivots, SingleVertex) {
  Graph g(1);
  
  DistState st = make_state(1);
  std::vector<int> S = {0};
  initialize_sources(st, S);
  
  FindPivotsParams params{2};
  uint64_t B = std::numeric_limits<uint64_t>::max();
  auto result = find_pivots(B, S, g, st, params);
  
  // W should at least contain the source
  EXPECT_GE(result.W.size(), 1u);
  EXPECT_NE(std::find(result.W.begin(), result.W.end(), 0), result.W.end());
}
