#include <gtest/gtest.h>

#include <queue>
#include <vector>
#include <random>

#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

namespace {
struct RefEdge { int to; uint64_t w; };

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
      int v = e.to; uint64_t nextDist = du + e.w;
      if (nextDist < dist[v]) { dist[v] = nextDist; pq.emplace(nextDist, v); }
    }
  }
  return dist;
}
}

static void run_case(std::size_t n, int seed) {
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

  // Use the public run_sssp API which auto-computes correct parameters
  auto got = run_sssp(g, s);

  ASSERT_EQ(got.size(), n);
  for (std::size_t i = 0; i < n; ++i) {
    EXPECT_EQ(got[i], ref[i]) << "seed=" << seed << " n=" << n << " vertex=" << i;
  }
}

TEST(BMSSPRecursion, SmallGraphsMatchDijkstra) {
  std::vector<int> seeds{2,3,5,7,11};
  for (int seed : seeds) {
    run_case(64, seed);
    run_case(128, seed);
  }
}
