#include <gtest/gtest.h>

#include <queue>
#include <vector>

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

  DistState st; st.reset(n);
  initialize_source(st, s);

  // Parameters: small k,t and few levels for tiny graphs
  std::size_t k = std::max<std::size_t>(1, static_cast<std::size_t>(std::cbrt(std::max<std::size_t>(n,1)))) ;
  std::size_t t = std::max<std::size_t>(1, static_cast<std::size_t>(std::sqrt(std::max<std::size_t>(n,1)))) ;
  int l = 2;
  BMSSPParams p{ k, t, l };

  std::vector<int> S = { s };
  uint64_t B = std::numeric_limits<uint64_t>::max();
  BMSSPResult r = bmssp(l, B, S, g, st, p);

  // Collect distances for vertices we completed (U plus W' already added by bmssp)
  std::vector<uint64_t> got(n, std::numeric_limits<uint64_t>::max());
  for (std::size_t i=0;i<n;++i) {
    if (!st.dist[i].is_inf()) {
      if (st.complete[i]) got[i] = st.dist[i].as_u64_clamped();
    }
  }

  for (std::size_t i=0; i<n; ++i) {
    // When bmssp succeeds fully, we expect exact match; on partial, all discovered (< boundary) must match
    if (r.boundary == B) {
      EXPECT_EQ(got[i], ref[i]) << "vertex " << i;
    } else {
      if (got[i] != std::numeric_limits<uint64_t>::max() && got[i] < r.boundary) {
        EXPECT_EQ(got[i], ref[i]) << "vertex " << i;
      }
    }
  }
}

TEST(BMSSPRecursion, SmallGraphsMatchDijkstra) {
  std::vector<int> seeds{2,3,5,7,11};
  for (int seed : seeds) {
    run_case(64, seed);
    run_case(128, seed);
  }
}
