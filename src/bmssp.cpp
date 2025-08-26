#include "cpp_starter/bmssp/bmssp.hpp"

namespace bmssp {

BMSSPResult bmssp(int l, uint64_t B, const std::vector<int>& S, Graph& g, DistState& st,
                  const BMSSPParams& p) {
  (void)l;  // recursion to be implemented in Phase 6
  BaseCaseParams bp{p.k};
  auto r = base_case(B, S, g, st, bp);
  return {r.boundary, r.U};
}

}  // namespace bmssp
