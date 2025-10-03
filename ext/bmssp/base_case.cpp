// ============================================================================
// Phase 5: BaseCase (Algorithm 2 from paper)
// ============================================================================
//
// Paper Reference: "Breaking the Sorting Barrier for Directed Single-Source
//                   Shortest Paths" (arXiv:2504.17033), Algorithm 2
//
// Purpose: Solve BMSSP for the base case (l = 0) using a mini-Dijkstra limited
//          by boundary B and returning at most k+1 vertices.
//
// Algorithm Overview (Algorithm 2):
//   Preconditions: |S| = 1, source x is complete (dist[x] already optimal)
//   1. Initialize priority queue Q with vertices reachable from x
//   2. Extract up to k+1 vertices with dist < B
//   3. If collected ≤ k vertices: return (B, U₀)
//   4. If collected = k+1 vertices: return (max dist in U₀, U₀ \ {k+1'th vertex})
//   5. Mark all returned vertices as complete
//
// Invariants:
//   - |U| ≤ k
//   - All u ∈ U have dist[u] < B' (returned boundary)
//   - All u ∈ U are marked complete
//
// ============================================================================

#include <queue>
#include <cassert>
#include <utility>

#include "cpp_starter/bmssp/base_case.hpp"

namespace bmssp {

namespace {
struct PQItem {
  DistWord d;
  int v;
};
struct Cmp {
  bool operator()(const PQItem& a, const PQItem& b) const {
    // priority_queue is max-heap by default; return true when a should come after b
    return compare(a.d, b.d) > 0;  // a.d > b.d means a has lower priority
  }
};
}

BaseCaseResult base_case(uint64_t B, const std::vector<int>& S, Graph& g, DistState& st,
                         const BaseCaseParams& p) {
  BaseCaseResult res{B, {}};
  if (S.empty() || p.k == 0) return res;  // trivial: nothing to explore
#ifdef ENABLE_BMSSP_VERIFIER
  // Preconditions: |S| == 1, source complete
  assert(S.size() == 1 && "BaseCase expects exactly one source");
  if (!S.empty()) {
    assert(is_complete(st, S[0]) && "BaseCase source must be marked complete");
  }
#endif

  // Initialize min-heap with sources whose distance is < B
  std::priority_queue<PQItem, std::vector<PQItem>, Cmp> pq;
  DistWord bound = DistWord::from_u64(B);
  for (int s : S) {
    if (s < 0) continue;
    std::size_t ss = static_cast<std::size_t>(s);
    if (ss >= st.dist.size()) continue;
    if (compare(st.dist[ss], bound) < 0) {
      pq.push({st.dist[ss], s});
    }
  }

  std::vector<int> U0;
  const std::size_t cap = p.k + 1;  // need k+1 to decide boundary

  while (!pq.empty() && U0.size() < cap) {
    PQItem it = pq.top();
    pq.pop();
    int u = it.v;
    std::size_t uu = static_cast<std::size_t>(u);
    if (uu >= st.dist.size()) continue;
    // stale check
    if (compare(it.d, st.dist[uu]) != 0) continue;
    U0.push_back(u);
    // relax from u to neighbors; only enqueue if new dist < B
    for (const auto& e : g.neighbors(u)) {
      int v = e.to;
      if (v < 0) continue;
      std::size_t vv = static_cast<std::size_t>(v);
      if (vv >= st.dist.size()) continue;
      if (relax(st, u, v, e.w, true)) {
        if (compare(st.dist[vv], bound) < 0) {
          pq.push({st.dist[vv], v});
        }
      }
    }
  }

  if (U0.size() <= p.k) {
    // boundary remains B; mark complete and return U0
    for (int v : U0) mark_complete(st, v);
    res.U = std::move(U0);
#ifdef ENABLE_BMSSP_VERIFIER
    // Invariants: all U complete; dist[v] < B; boundary unchanged
    DistWord bound_check = DistWord::from_u64(B);
    for (int v : res.U) {
      assert(is_complete(st, v));
      std::size_t vv = static_cast<std::size_t>(v);
      if (vv < st.dist.size()) {
        assert(compare(st.dist[vv], bound_check) < 0);
      }
    }
    assert(res.boundary == B);
#endif
    return res;
  }

  // Determine boundary as max distance in U0; then U are those strictly below boundary
  DistWord boundary_dw = DistWord::from_u64(0);
  bool boundary_init = false;
  for (int v : U0) {
    std::size_t vv = static_cast<std::size_t>(v);
    if (vv >= st.dist.size()) continue;
    if (!boundary_init || compare(st.dist[vv], boundary_dw) > 0) {
      boundary_dw = st.dist[vv];
      boundary_init = true;
    }
  }
  std::vector<int> U;
  for (int v : U0) {
    std::size_t vv = static_cast<std::size_t>(v);
    if (vv >= st.dist.size()) continue;
    if (compare(st.dist[vv], boundary_dw) < 0) U.push_back(v);
  }
  for (int v : U) mark_complete(st, v);

  // Clamp boundary to u64 for result type
  res.boundary = st.dist.empty() ? B : DistWord::from_u64(0).as_u64_clamped();
  // Convert boundary_dw to uint64_t safely
  // Prefer: if boundary_dw fits in u64, use exact; else clamp to max
  switch (boundary_dw.width) {
    case DistWidth::W32:
      res.boundary = boundary_dw.small.v32;
      break;
    case DistWidth::W64:
      res.boundary = boundary_dw.small.v64;
      break;
    case DistWidth::W128:
      res.boundary = static_cast<uint64_t>(boundary_dw.small.v128);
      break;
    case DistWidth::BIG:
      res.boundary = boundary_dw.big.limbs.empty() ? 0 : boundary_dw.big.limbs[0];
      break;
  }
  res.U = std::move(U);

#ifdef ENABLE_BMSSP_VERIFIER
  // Invariants for >k case:
  // - |U| <= k; all U complete; dist[v] < boundary and < B
  // - boundary equals max dist in U0; U0 has at least one at boundary
  assert(res.U.size() <= p.k);
  bool found_eq_boundary = false;
  DistWord bound_check = DistWord::from_u64(B);
  for (int v : res.U) {
    assert(is_complete(st, v));
    std::size_t vv = static_cast<std::size_t>(v);
    if (vv < st.dist.size()) {
      assert(compare(st.dist[vv], boundary_dw) < 0);
      assert(compare(st.dist[vv], bound_check) < 0);
    }
  }
  for (int v : U0) {
    std::size_t vv = static_cast<std::size_t>(v);
    if (vv < st.dist.size()) {
      if (compare(st.dist[vv], boundary_dw) == 0) found_eq_boundary = true;
    }
  }
  assert(found_eq_boundary && "Expected at least one vertex at boundary among first k+1 visited");
#endif
  return res;
}

}  // namespace bmssp
