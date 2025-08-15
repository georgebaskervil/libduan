#include <gtest/gtest.h>

#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

TEST(BMSSPBoundary, Near2Pow32) {
  DistState st = make_state(3);
  // Start u near 2^32-1 to force 32->64 widening when MIN_DISTANCE_BITS=32
#if MIN_DISTANCE_BITS == 32
  st.dist[0].width = DistWidth::W32;
  st.dist[0].small.v32 = 0xFFFFFFFFu - 5u;
#else
  st.dist[0].width = DistWidth::W64;
  st.dist[0].small.v64 = (1ULL << 32) - 5ULL;
#endif
  st.pred[0] = 0;
  st.hop[0] = 0;
  st.complete[0] = 1;
  st.dist[1] = DistWord::inf();
  st.pred[1] = -1;
  st.hop[1] = 0;
  bool widened = false;
  EXPECT_TRUE(relax(st, 0, 1, 10ULL, true, &widened));
}

TEST(BMSSPBoundary, Near2Pow64) {
  DistState st = make_state(3);
  st.dist[0].width = DistWidth::W64;
  st.dist[0].small.v64 = std::numeric_limits<uint64_t>::max() - 5ULL;
  st.pred[0] = 0;
  st.hop[0] = 0;
  st.complete[0] = 1;
  st.dist[1] = DistWord::inf();
  st.pred[1] = -1;
  st.hop[1] = 0;
  bool widened = false;
  EXPECT_TRUE(relax(st, 0, 1, 10ULL, true, &widened));
}

TEST(BMSSPInvariants, CompletedVertexUnchangedAfterWiden) {
  DistState st = make_state(2);
  // Mark v complete with finite value. Widening elsewhere must not change it.
  st.dist[1].width = DistWidth::W64;
  st.dist[1].small.v64 = 1234ULL;
  st.pred[1] = 1;
  st.hop[1] = 0;
  st.complete[1] = 1;

  st.dist[0].width = DistWidth::W64;
  st.dist[0].small.v64 = std::numeric_limits<uint64_t>::max() - 2ULL;
  st.pred[0] = 0;
  st.hop[0] = 0;

  bool widened = false;
  (void)relax(st, 0, 0, 10ULL, true, &widened);  // self-edge to trigger widening

  // Check v unchanged
  EXPECT_EQ(st.dist[1].width, DistWidth::W64);
  EXPECT_EQ(st.dist[1].small.v64, 1234ULL);
}
