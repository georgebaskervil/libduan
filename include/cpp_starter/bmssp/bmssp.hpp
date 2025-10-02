#pragma once
#include <cstdint>
#include <vector>

#include "base_case.hpp"
#include "find_pivots.hpp"
#include "graph.hpp"
#include "state.hpp"
#include "structure.hpp"

namespace bmssp {

struct BMSSPParams {
  std::size_t k;
  std::size_t t;
  int level;
};
struct BMSSPResult {
  uint64_t boundary;
  std::vector<int> U;
};

BMSSPResult bmssp(int l, uint64_t B, const std::vector<int>& S, Graph& g, DistState& st,
                  const BMSSPParams& p);

// Top-level SSSP driver: auto-computes parameters and runs BMSSP
// Returns distance vector for all vertices from source s
std::vector<uint64_t> run_sssp(const Graph& g, int source);

}  // namespace bmssp
