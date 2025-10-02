#pragma once
#include <cstddef>

namespace bmssp {

struct DistState;

// Dump performance statistics from a DistState
// Only functional when ENABLE_BMSSP_VERIFIER is defined
void dump_stats(const DistState& st, std::size_t n_vertices, std::size_t n_edges);

}  // namespace bmssp
