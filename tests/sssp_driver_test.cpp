#include <gtest/gtest.h>

#include <queue>
#include <vector>
#include <random>

#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"

using namespace bmssp;

namespace {
std::vector<uint64_t> dijkstra_ref(const Graph& g, int s) {
  const std::size_t n = g.size();
  const uint64_t INF = std::numeric_limits<uint64_t>::max()/4;
  std::vector<uint64_t> dist(n, INF);
  using QN = std::pair<uint64_t,int>;
  std::priority_queue<QN, std::vector<QN>, std::greater<QN>> pq;
  dist[s] = 0; pq.emplace(0, s);
  while (!pq.empty()) {
    auto [du,u] = pq.top(); pq.pop();
    if (du != dist[u]) continue;
    for (const auto& e : g.neighbors(u)) {
      int v = e.to; uint64_t nd = du + e.w;
      if (nd < dist[v]) { dist[v] = nd; pq.emplace(nd, v); }
    }
  }
  return dist;
}
}

TEST(BMSSPDriver, TrivialSingleVertex) {
  Graph g(1);
  auto dist = run_sssp(g, 0);
  ASSERT_EQ(dist.size(), 1u);
  EXPECT_EQ(dist[0], 0u);
}

TEST(BMSSPDriver, SimpleLine) {
  Graph g(4);
  g.add_edge(0, 1, 10);
  g.add_edge(1, 2, 20);
  g.add_edge(2, 3, 30);
  
  auto dist = run_sssp(g, 0);
  ASSERT_EQ(dist.size(), 4u);
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 10u);
  EXPECT_EQ(dist[2], 30u);
  EXPECT_EQ(dist[3], 60u);
}

TEST(BMSSPDriver, RandomGraphsMatchDijkstra) {
  std::vector<int> seeds{2,3,5,7,11};
  std::vector<std::size_t> sizes{32, 64, 128};
  
  for (int seed : seeds) {
    for (std::size_t n : sizes) {
      Graph g(n);
      std::mt19937 rng(seed);
      std::uniform_int_distribution<int> deg(0,2);
      std::uniform_int_distribution<int> vdist(0, static_cast<int>(n)-1);
      std::uniform_int_distribution<uint64_t> wdist(1, 10);
      
      for (std::size_t u=0; u<n; ++u) {
        int d = deg(rng);
        for (int i=0; i<d; ++i) {
          int v = vdist(rng);
          if (v == static_cast<int>(u)) continue;
          g.add_edge(static_cast<int>(u), v, wdist(rng));
        }
      }
      
      int s = 0;
      auto ref = dijkstra_ref(g, s);
      auto got = run_sssp(g, s);
      
      ASSERT_EQ(got.size(), n);
      for (std::size_t i=0; i<n; ++i) {
        EXPECT_EQ(got[i], ref[i]) << "seed=" << seed << " n=" << n << " vertex=" << i;
      }
    }
  }
}

TEST(BMSSPDriver, DisconnectedGraph) {
  Graph g(5);
  g.add_edge(0, 1, 5);
  g.add_edge(1, 2, 10);
  // 3 and 4 are disconnected
  
  const uint64_t INF = std::numeric_limits<uint64_t>::max() / 4;
  auto dist = run_sssp(g, 0);
  ASSERT_EQ(dist.size(), 5u);
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 5u);
  EXPECT_EQ(dist[2], 15u);
  EXPECT_EQ(dist[3], INF);
  EXPECT_EQ(dist[4], INF);
}

TEST(BMSSPDriver, InvalidSource) {
  Graph g(3);
  g.add_edge(0, 1, 5);
  
  // Invalid source should return all vertices at INF (max/4)
  const uint64_t INF = std::numeric_limits<uint64_t>::max() / 4;
  auto dist = run_sssp(g, -1);
  ASSERT_EQ(dist.size(), 3u);
  for (auto d : dist) {
    EXPECT_EQ(d, INF);
  }
  
  dist = run_sssp(g, 10);
  ASSERT_EQ(dist.size(), 3u);
  for (auto d : dist) {
    EXPECT_EQ(d, INF);
  }
}

TEST(BMSSPDriver, EmptyGraph) {
  Graph g(0);
  auto dist = run_sssp(g, 0);
  EXPECT_TRUE(dist.empty());
}
