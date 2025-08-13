#pragma once
#include <vector>
#include <cstdint>
#include <limits>

namespace bmssp {

// Placeholder widening distance word (Phase 2 will implement widening logic)
struct DistState {
  std::vector<unsigned __int128> dist; // temporary superset type
  std::vector<int> pred;
  std::vector<int> hop;
  std::vector<unsigned char> complete;
};

inline unsigned __int128 inf128() { return (unsigned __int128)(~(unsigned long long)0); }

inline DistState make_state(std::size_t n) {
  DistState s;
  s.dist.assign(n, inf128());
  s.pred.assign(n, -1);
  s.hop.assign(n, 0);
  s.complete.assign(n, 0);
  return s;
}

} // namespace bmssp
