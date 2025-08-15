#pragma once
#include <cstdint>
#include <vector>

#include "graph.hpp"
#include "state.hpp"

namespace bmssp {

struct FindPivotsParams {
  std::size_t k;
};
struct PivotResult {
  std::vector<int> P;
  std::vector<int> W;
};

// Phase 3: will implement full Algorithm 1; this is a placeholder signature.
PivotResult find_pivots(uint64_t B, const std::vector<int>& S, Graph& g, DistState& st,
                        const FindPivotsParams& p);

}  // namespace bmssp
