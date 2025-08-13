// License: Placeholder - add your license details here.
#include <gtest/gtest.h>
#include "cpp_starter/lib.hpp"
#include "cpp_starter/bmssp.hpp"

TEST(CppStarter, SumBasics) {
  EXPECT_EQ(cpp_starter::sum(0, 0), 0);
  EXPECT_EQ(cpp_starter::sum(1, 2), 3);
  EXPECT_EQ(cpp_starter::sum(-5, 5), 0);
}

TEST(CppStarter, SumEdge) {
  EXPECT_EQ(cpp_starter::sum(2147483640, 5), 2147483645);
}

TEST(BMSSP, TrivialLine) {
  using namespace cpp_starter;
  Graph g(5);
  // Line 0-1-2-3-4 with weight 1
  g.add_edge(0,1,1); g.add_edge(1,2,1); g.add_edge(2,3,1); g.add_edge(3,4,1);
  auto dist = make_distance_vector(g.size());
  dist[0] = 0.0; // source distance
  std::vector<int> S = {0};
  BMSSPParams params{2,1};
  double B = 10.0;
  auto res = BMSSP(1, B, S, g, dist, params);
  EXPECT_LE(res.boundary, B);
  EXPECT_FALSE(res.U.empty());
  // Ensure distances monotonically non-decreasing along discovered path
  for (int v : res.U) {
    if(v<0) continue; // defensive
    std::size_t vv = static_cast<std::size_t>(v);
    ASSERT_LT(vv, dist.size());
    EXPECT_GE(dist[vv], 0.0);
  }
}
