// Minimal BMSSP implementation (clean version without legacy warnings)
// Provides trivial but functional versions of FindPivots, BaseCase, and BMSSP
// sufficient for unit tests and iterative development. Focus: simplicity &
// warning-free compilation.

#include <functional>
#include <limits>
#include <queue>

#include "cpp_starter/bmssp.hpp"

namespace cpp_starter {

PivotResult FindPivots(const Graph& /*g*/, double /*B*/, const std::vector<int>& S,
                       std::vector<double>& /*dist*/, const BMSSPParams& /*p*/) {
  // Trivial: treat sources themselves as pivots; explored set W=S
  return {S, S};
}

BMSSPResult BaseCase(double B, const std::vector<int>& S, const Graph& g, std::vector<double>& dist,
                     const BMSSPParams& p) {
  if (S.empty()) return {B, {}};
  const std::size_t k = p.k;
  using QItem = std::pair<double, int>;  // (dist, vertex)
  std::priority_queue<QItem, std::vector<QItem>, std::greater<QItem>> pq;
  for (int s : S) {
    std::size_t ss = static_cast<std::size_t>(s);
    if (ss < dist.size() && dist[ss] < B) pq.emplace(dist[ss], s);
  }
  std::vector<int> visited;
  const std::size_t visit_cap = k + 1;  // need k+1 to decide boundary
  while (!pq.empty() && visited.size() < visit_cap) {
    auto [du, u] = pq.top();
    pq.pop();
    std::size_t uu = static_cast<std::size_t>(u);
    if (uu >= dist.size() || du != dist[uu]) continue;  // stale
    visited.push_back(u);
    for (const auto& e : g.neighbors(uu)) {
      double cand = du + e.w;
      if (cand < dist[e.to] && cand < B) {
        dist[e.to] = cand;
        pq.emplace(cand, static_cast<int>(e.to));
      }
    }
  }
  if (visited.size() <= k) return {B, visited};
  double boundary = 0.0;
  for (int v : visited) {
    std::size_t vv = static_cast<std::size_t>(v);
    if (vv < dist.size() && dist[vv] > boundary) boundary = dist[vv];
  }
  std::vector<int> U;
  for (int v : visited) {
    std::size_t vv = static_cast<std::size_t>(v);
    if (vv < dist.size() && dist[vv] < boundary) U.push_back(v);
  }
  return {boundary, U};
}

BMSSPResult BMSSP(int l, double B, const std::vector<int>& S, const Graph& g,
                  std::vector<double>& dist, const BMSSPParams& p) {
  (void)l;  // recursion not yet used in simplified version
  return BaseCase(B, S, g, dist, p);
}

}  // namespace cpp_starter
