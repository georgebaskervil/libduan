#include "cpp_starter/bmssp/degree_check.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include <iostream>
#include <cstdlib>

namespace bmssp {

std::pair<std::size_t, std::size_t> compute_max_degrees(const Graph& g) {
  std::size_t max_out = 0;
  std::size_t max_in = 0;
  
  std::size_t n = g.size();
  std::vector<std::size_t> in_degree(n, 0);
  
  // Compute out-degrees and in-degrees
  for (std::size_t u = 0; u < n; ++u) {
    std::size_t out_deg = g.neighbors(static_cast<int>(u)).size();
    max_out = std::max(max_out, out_deg);
    
    for (const auto& e : g.neighbors(static_cast<int>(u))) {
      if (e.to >= 0 && static_cast<std::size_t>(e.to) < n) {
        in_degree[static_cast<std::size_t>(e.to)]++;
      }
    }
  }
  
  for (std::size_t deg : in_degree) {
    max_in = std::max(max_in, deg);
  }
  
  return {max_out, max_in};
}

void validate_degree_constraint(const Graph& g) {
  auto [max_out, max_in] = compute_max_degrees(g);
  std::size_t max_deg = std::max(max_out, max_in);
  
#ifdef ENABLE_BMSSP_VERIFIER
  if (max_deg > 2) {
    std::cerr << "WARNING: Graph has max degree " << max_deg << " (out: " << max_out 
              << ", in: " << max_in << "). Paper assumes constant degree ≤ 2.\n";
    std::cerr << "         The O(m + n log^{2/3} n) bound may not hold.\n";
    
#ifdef CONST_DEGREE_STRICT_MODE
    std::cerr << "CONST_DEGREE_STRICT_MODE: Aborting due to degree > 2.\n";
    std::abort();
#endif
  }
#else
  // Suppress unused variable warnings when verifier is off
  (void)max_out;
  (void)max_in;
  (void)max_deg;
#endif
}

}  // namespace bmssp
