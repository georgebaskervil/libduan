#pragma once
#include <vector>
#include <cstdint>

namespace bmssp {
struct VerificationStats {
  std::size_t relax_equal = 0;
  std::size_t relax_improve = 0;
  std::size_t inserts = 0;
  std::size_t batch_prepends = 0;
  std::size_t pulls = 0;
};
}
