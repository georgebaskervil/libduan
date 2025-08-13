#pragma once
#include <vector>
#include <cstdint>
#include "graph.hpp"
#include "state.hpp"

namespace bmssp {

struct BaseCaseParams { std::size_t k; };
struct BaseCaseResult { uint64_t boundary; std::vector<int> U; };

BaseCaseResult base_case(uint64_t B, const std::vector<int>& S, Graph& g, DistState& st, const BaseCaseParams& p);

} // namespace bmssp
