#pragma once
#include <vector>
#include <cstdint>
#include "graph.hpp"
#include "state.hpp"
#include "structure.hpp"
#include "find_pivots.hpp"
#include "base_case.hpp"

namespace bmssp {

struct BMSSPParams { std::size_t k; std::size_t t; int level; };
struct BMSSPResult { uint64_t boundary; std::vector<int> U; };

BMSSPResult bmssp(int l, uint64_t B, const std::vector<int>& S, Graph& g, DistState& st, const BMSSPParams& p);

} // namespace bmssp
