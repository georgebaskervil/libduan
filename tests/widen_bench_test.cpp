#include <gtest/gtest.h>

#include <random>

#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

#ifdef ENABLE_BMSSP_VERIFIER
TEST(BMSSPWidenBench, MeasureWideningOverhead) {
  const int n = 10000;
  DistState st = make_state(n);
  // Set fastpath to a lower bound so some calls still go through overflow check
  st.fastpath_u64_max_sum = std::numeric_limits<uint64_t>::max() - 1ULL;
  // Create a chain to force many relax calls; weights near boundary to cause some widenings when
  // FLAGS differ
  std::vector<uint64_t> w(static_cast<size_t>(n - 1), 1ULL);
  // Seed u at large value so additions will trigger widening after some steps
  st.dist[0].width = DistWidth::W64;
  st.dist[0].small.v64 = std::numeric_limits<uint64_t>::max() - 1000ULL;
  st.pred[0] = 0;
  st.hop[0] = 0;
  bool widened = false;
  (void)widened;
  for (int i = 0; i < n - 1; ++i) {
    (void)relax(st, i, i + 1, w[static_cast<size_t>(i)], true, &widened);
  }
  // Print counters (visible when running with ctest -V)
  std::cout << "widen_events=" << st.widen_events << " overflow_events=" << st.overflow_events
            << " widen_bytes=" << st.widen_bytes << " big_bytes=" << st.big_bytes
            << " used_bigint=" << st.used_bigint << std::endl;
  SUCCEED();
}
#endif
