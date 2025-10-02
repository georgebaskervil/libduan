#pragma once
#include <cstddef>
#include <utility>  // for std::pair

namespace bmssp {

class Graph;

// Compute max out-degree and in-degree of a graph
std::pair<std::size_t, std::size_t> compute_max_degrees(const Graph& g);

// Check if graph satisfies constant-degree assumption (max degree ≤ 2)
// When ENABLE_BMSSP_VERIFIER is on, warns if degree > 2
// When CONST_DEGREE_STRICT_MODE is on, aborts if degree > 2
void validate_degree_constraint(const Graph& g);

}  // namespace bmssp
