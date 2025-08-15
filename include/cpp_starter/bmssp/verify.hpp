#pragma once
#include <cstdint>
#include <vector>

namespace bmssp {
struct VerificationStats {
  std::size_t relax_equal = 0;
  std::size_t relax_improve = 0;
  std::size_t inserts = 0;
  std::size_t batch_prepends = 0;
  std::size_t pulls = 0;
};
}  // namespace bmssp
