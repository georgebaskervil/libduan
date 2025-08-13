/* BMSSP (Bounded Multi-Source Shortest Path) prototype implementation
 * Adapted from the pseudocode (Algorithms 1–3) in:
 *   Bounded Multi-Source Shortest Paths (arXiv:2504.17033)
 * This is an experimental, simplified translation to C++.
 * Assumptions / Simplifications:
 *  - Edge weights are non‑negative (Dijkstra‑style semantics required).
 *  - Graph stored as adjacency list; vertices are 0..(n-1).
 *  - We treat “complete” vs “incomplete” implicitly: the caller supplies S (complete set) and a distance vector initialized for sources.
 *  - Forest / pivot logic is approximated; subtree sizes are estimated via a DFS inside W.
 *  - Data structure 𝒟 (Lemma 3.3) is approximated with a binary min-heap (std::priority_queue).
 *  - This implementation focuses on correctness of the returned boundary B' ≤ B and a candidate set U; performance / theoretical bounds may differ from the paper.
 *  - Parameters k and t are provided by caller inside BMSSPParams; we don’t derive them.
 *  - All distances stored as double; adapt if integral weights are guaranteed.
 */

#pragma once

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>
#include <unordered_set>

namespace cpp_starter {

struct Edge { int to; double w; };

class Graph {
public:
  Graph() = default;
  explicit Graph(std::size_t n) : adj_(n) {}
  std::size_t size() const noexcept { return adj_.size(); }
  void add_edge(int u, int v, double w) { adj_[u].push_back({v, w}); }
  const std::vector<Edge>& neighbors(int u) const { return adj_[u]; }
private:
  std::vector<std::vector<Edge>> adj_;
};

struct BMSSPParams {
  std::size_t k; // threshold parameter (see paper)
  std::size_t t; // scaling parameter used with recursion limit l
};

struct PivotResult {
  std::vector<int> P; // pivots
  std::vector<int> W; // explored union set
};

// FindPivots simplified implementation
PivotResult FindPivots(const Graph& g, double B, const std::vector<int>& S,
                       std::vector<double>& dist, const BMSSPParams& p);

struct BMSSPResult {
  double boundary;       // B'
  std::vector<int> U;    // set U
};

// BaseCase (Algorithm 2 simplified). Expects |S| == 1, S[0] complete.
BMSSPResult BaseCase(double B, const std::vector<int>& S, const Graph& g,
                     std::vector<double>& dist, const BMSSPParams& p);

// Recursive BMSSP (Algorithm 3 simplified).
BMSSPResult BMSSP(int l, double B, const std::vector<int>& S, const Graph& g,
                  std::vector<double>& dist, const BMSSPParams& p);

// Utility: create initialized distance vector.
inline std::vector<double> make_distance_vector(std::size_t n) {
  return std::vector<double>(n, std::numeric_limits<double>::infinity());
}

} // namespace cpp_starter
