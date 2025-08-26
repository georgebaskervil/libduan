// Phase 3: Faithful-ish FindPivots (Algorithm 1) implementation
// - Layered relaxations for exactly k iterations or until frontier exhausts
// - Maintains W_i layers and their union W (including S)
// - Early exit if |W| > k |S| returning P=S
// - Builds shortest-path forest over tight edges within W and selects pivots

#include <algorithm>
#include <cassert>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "cpp_starter/bmssp/find_pivots.hpp"

namespace bmssp {

namespace {
// Compare a DistWord with a u64 boundary. Return true if dw < B.
inline bool dist_less_than_bound(const DistWord& dw, uint64_t B) {
  DistWord Bw = DistWord::from_u64(B);
  return compare(dw, Bw) < 0;
}

struct Cand {
  enum Kind { K128, KBIG } kind{K128};
  unsigned __int128 v128{0};
  BigInt vbig;
};

// Form candidate distance du + w in either 128-bit or BigInt space.
inline Cand make_candidate(const DistWord& du, uint64_t w) {
  Cand c;
  if (du.width == DistWidth::W32 || du.width == DistWidth::W64) {
    uint64_t base_u = (du.width == DistWidth::W32) ? (uint64_t)du.small.v32 : du.small.v64;
    c.v128 = (unsigned __int128)base_u + (unsigned __int128)w;  // may exceed 64, ok in 128
    c.kind = Cand::K128;
  } else if (du.width == DistWidth::W128) {
    c.v128 = du.small.v128 + (unsigned __int128)w;
    c.kind = Cand::K128;
  } else {  // BIG
#ifdef ENABLE_BIGINT_FALLBACK
    c.vbig = du.big;
    c.vbig.add_u64_inplace(w);
    c.kind = Cand::KBIG;
#else
    // Fallback disabled; approximate in 128 bits (for debug builds without BIGINT)
    unsigned __int128 approx = 0;
    for (std::size_t i = du.big.limbs.size(); i-- > 0;) {
      approx <<= 64;
      approx |= du.big.limbs[i];
    }
    c.v128 = approx + (unsigned __int128)w;
    c.kind = Cand::K128;
#endif
  }
  return c;
}

inline bool candidate_lt_bound(const Cand& c, uint64_t B) {
  if (c.kind == Cand::KBIG) {
#ifdef ENABLE_BIGINT_FALLBACK
    BigInt bb;
    bb.assign(B);
    return BigInt::cmp(c.vbig, bb) < 0;
#else
    return false;
#endif
  } else {
    return c.v128 < (unsigned __int128)B;
  }
}

inline bool candidate_equals_dv(const Cand& c, const DistWord& dv) {
  if (c.kind == Cand::KBIG) {
#ifdef ENABLE_BIGINT_FALLBACK
    if (dv.width == DistWidth::BIG) return BigInt::cmp(c.vbig, dv.big) == 0;
    BigInt cur = distword_to_big(dv);
    return BigInt::cmp(c.vbig, cur) == 0;
#else
    return false;
#endif
  } else {
    if (dv.width == DistWidth::BIG) {
#ifdef ENABLE_BIGINT_FALLBACK
      BigInt tmp;
      tmp.assign_u128(c.v128);
      return BigInt::cmp(tmp, dv.big) == 0;
#else
      return false;
#endif
    }
    unsigned __int128 dv128 = 0;
    if (dv.width == DistWidth::W32)
      dv128 = dv.small.v32;
    else if (dv.width == DistWidth::W64)
      dv128 = dv.small.v64;
    else
      dv128 = dv.small.v128;
    return c.v128 == dv128;
  }
}

}  // namespace

PivotResult find_pivots(uint64_t B, const std::vector<int>& S, Graph& g, DistState& st,
                        const FindPivotsParams& p) {
  const std::size_t n = g.size();
  const std::size_t k = p.k;
  PivotResult res;
  res.P.clear();
  res.W.clear();
  if (S.empty() || n == 0) return res;

  // Layered relaxation up to k iterations
  std::vector<uint8_t> inW(n, 0);
  std::vector<int> W;  // union of layers
  W.reserve(std::min(n, k * std::max<std::size_t>(S.size(), 1ULL)) + S.size());
  std::vector<int> cur = S;
  for (int s : S) {
    if (s >= 0 && (std::size_t)s < n && !inW[(std::size_t)s]) {
      inW[(std::size_t)s] = 1;
      W.push_back(s);
    }
  }
  std::vector<int> next;
  next.reserve(2 * S.size());

  for (std::size_t it = 0; it < k && !cur.empty(); ++it) {
    next.clear();
    for (int u : cur) {
      if (u < 0) continue;
      std::size_t su = (std::size_t)u;
      if (su >= n) continue;
      const auto& nbrs = g.neighbors(u);
      for (const auto& e : nbrs) {
        int v = e.to;
        if (v < 0 || (std::size_t)v >= n) continue;
  // Gate by boundary B using candidate distance first
  const DistWord& du = st.dist[su];
  if (du.is_inf()) continue;
  Cand cand = make_candidate(du, e.w);
  if (!candidate_lt_bound(cand, B)) continue;
  bool improved = relax(st, u, v, e.w, true, nullptr);
        if (!improved) continue;
  // Add v to next layer; we already ensured candidate < B
  if (dist_less_than_bound(st.dist[(std::size_t)v], B)) {
          if (!inW[(std::size_t)v]) {
            inW[(std::size_t)v] = 1;
            W.push_back(v);
          }
          next.push_back(v);
        }
      }
    }
    // Early exit: |W| > k |S| => return P=S
    if (W.size() > k * S.size()) {
      res.P = S;  // P subset of S and equals S under early exit
      res.W = W;
      return res;
    }
    cur.swap(next);
  }

  // Build a parent forest over tight edges within W
  // parent[v] = chosen parent in W (or -1 for roots)
  std::vector<int> parent(n, -1);
  std::vector<uint8_t> inWmask = inW;  // copy for const-like usage

  // For each u in W, scan outgoing tight edges to v in W and pick parent for v
  for (int u : W) {
    if (u < 0) continue;
    std::size_t su = (std::size_t)u;
    if (su >= n) continue;
    const auto& nbrs = g.neighbors(u);
    for (const auto& e : nbrs) {
      int v = e.to;
      if (v < 0 || (std::size_t)v >= n) continue;
      if (!inWmask[(std::size_t)v]) continue;
      // Check tightness: dist[v] == dist[u] + w
      const DistWord& du = st.dist[su];
      const DistWord& dv = st.dist[(std::size_t)v];
      if (du.is_inf() || dv.is_inf()) continue;
      Cand cand = make_candidate(du, e.w);
      if (!candidate_equals_dv(cand, dv)) continue;
      // Candidate parent for v is u; choose best (min dist[u], then min id)
      int& pv = parent[(std::size_t)v];
      if (pv == -1) {
        pv = u;
      } else {
        const DistWord& puw = st.dist[(std::size_t)pv];
        int cmp = compare(st.dist[su], puw);
        if (cmp < 0 || (cmp == 0 && u < pv)) pv = u;
      }
    }
  }

  // Build children lists for nodes in W using chosen parents
  std::vector<std::vector<int>> children(n);
  children.shrink_to_fit();
  for (int v : W) {
    int pv = parent[(std::size_t)v];
    if (pv >= 0) children[(std::size_t)pv].push_back(v);
  }

  // Subtree sizes via DFS; count includes the node itself
  std::vector<int> subsize(n, 0);
  std::vector<uint8_t> seen(n, 0);
  std::vector<int> stk;
  std::vector<uint8_t> onstack(n, 0);
  for (int root : S) {
    if (root < 0 || (std::size_t)root >= n) continue;
    if (!inWmask[(std::size_t)root]) continue;
    // Iterative post-order DFS
    stk.clear();
    stk.push_back(root);
    while (!stk.empty()) {
      int u = stk.back();
      if (!onstack[(std::size_t)u]) {
        onstack[(std::size_t)u] = 1;
        for (int wv : children[(std::size_t)u]) {
          if (!onstack[(std::size_t)wv]) stk.push_back(wv);
        }
      } else {
        stk.pop_back();
        int total = 1;
        for (int wv : children[(std::size_t)u]) total += subsize[(std::size_t)wv];
        subsize[(std::size_t)u] = total;
      }
    }
    std::fill(onstack.begin(), onstack.end(), 0);
  }

  // Select pivots: roots in S with subtree size >= k
  for (int s : S) {
    if (s < 0 || (std::size_t)s >= n) continue;
    if (!inWmask[(std::size_t)s]) continue;
    if (subsize[(std::size_t)s] >= 0 && (std::size_t)subsize[(std::size_t)s] >= k) {
      res.P.push_back(s);
    }
  }

  // Produce W result (as collected)
  res.W = std::move(W);

  // Optional: ensure P subset S and bound |P| <= |W|/k
  if (!res.W.empty() && k > 0) {
    if (res.P.size() > res.W.size() / k) {
      // In the rare case our parent selection inflated counts, trim P by smallest subtree sizes
      std::vector<std::pair<int, int>> pv;  // (subsize, vertex)
      pv.reserve(res.P.size());
      for (int v : res.P) pv.emplace_back(subsize[(std::size_t)v], v);
      std::sort(pv.begin(), pv.end());
      std::size_t keep = res.W.size() / k;
      std::vector<int> trimmed;
      for (std::size_t i = pv.size() - std::min(pv.size(), keep); i < pv.size(); ++i)
        trimmed.push_back(pv[i].second);
      res.P.swap(trimmed);
    }
  }

#ifdef ENABLE_BMSSP_VERIFIER
  // Assert |W| <= k|S| when no early exit
  if (k > 0) {
    assert(res.W.size() <= k * S.size());
  }
#endif

  return res;
}

}  // namespace bmssp
