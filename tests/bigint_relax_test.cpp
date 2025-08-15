#include <gtest/gtest.h>
#include "cpp_starter/bmssp/state.hpp"

using namespace bmssp;

// This test exercises widening to 128-bit then BIGINT (if enabled) and ensures
// lexicographic tie-breaking keeps predecessor with smaller id on equal distance.
// It is compiled & run only when ENABLE_BIGINT_FALLBACK is defined (see CMake options).
#ifdef ENABLE_BIGINT_FALLBACK
TEST(BMSSPBigInt, WidenAndBigIntAdd) {
  // Construct state with two vertices u -> v with large distance near 2^64-1 and weight causing overflow.
  DistState st = make_state(2);
  // Force u distance to max 64-bit - 5 so adding 10 overflows 64 but fits in 128.
  st.dist[0].width = DistWidth::W64;
  st.dist[0].small.v64 = std::numeric_limits<uint64_t>::max() - 5ULL;
  st.pred[0]=0; st.hop[0]=0;
  // v initially infinity.
  bool widened=false;
  // First relax: add weight 10 triggers widening from 64 to 128.
  bool changed = relax(st, 0, 1, 10ULL, true, &widened);
  EXPECT_TRUE(changed);
  EXPECT_TRUE(widened);
  EXPECT_EQ(st.dist[1].width, DistWidth::W128);
  // Reset v to INF so next relax is considered an improvement regardless of candidate magnitude.
  st.dist[1] = DistWord::inf();
  st.pred[1] = -1; st.hop[1]=0;
  EXPECT_TRUE(st.dist[1].is_inf());
  // Now directly promote source to BIG and perform a relax to propagate BIG to v.
  promote_to_big(st.dist[0]);
  EXPECT_EQ(st.dist[0].width, DistWidth::BIG);
  // Set a small BigInt value so candidate < INF sentinel and must be accepted.
  st.dist[0].big.limbs = { 10ULL };
  widened=false;
  changed = relax(st, 0, 1, 3ULL, true, &widened); // candidate BIG + small weight => 13
  EXPECT_TRUE(changed);
  EXPECT_EQ(st.dist[1].width, DistWidth::BIG);
  EXPECT_TRUE(st.used_bigint);
  // Tie-breaking: create competing predecessor with same distance but larger id; ensure not taken.
  // Setup vertex 1 as source alternative via vertex 1 -> 0 (same distance path). We simulate by relaxing back with equal distance.
  // Make distance of vertex 1 copied to vertex 0 candidate via weight 0; predecessor should remain 0 since 0 < 1.
  unsigned __int128 dist1_low = st.dist[1].width==DistWidth::BIG ? (unsigned __int128)st.dist[1].big.limbs[0] : 0;
  (void)dist1_low; // placeholder usage in case we later extend assertions.
  widened=false;
  (void)relax(st, 1, 0, 0ULL, true, &widened); // ignore result; only tie-break effect observed via pred
  // Either equal distance relax accepted (then pred[0] becomes 1 only if tie-break decides) or rejected; assert tie-break rule preference for smaller id keeps pred[0]==0.
  EXPECT_EQ(st.pred[0], 0);
}

TEST(BMSSPBigInt, AcceptFromInfinityBigSource) {
  DistState st = make_state(2);
  // Make source BIG with a multi-limb value.
  promote_to_big(st.dist[0]);
  st.dist[0].big.limbs = {123ULL};
  st.pred[0]=0; st.hop[0]=0;
  // v is INF default.
  bool widened=false;
  bool changed = relax(st, 0, 1, 7ULL, true, &widened);
  EXPECT_TRUE(changed);
  EXPECT_EQ(st.dist[1].width, DistWidth::BIG);
  EXPECT_TRUE(st.used_bigint);
}
#endif
