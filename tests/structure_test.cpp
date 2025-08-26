#include <gtest/gtest.h>

#include "cpp_starter/bmssp/structure.hpp"

using namespace bmssp;

static unsigned __int128 u128(uint64_t v) { return (unsigned __int128)v; }

TEST(PartialPriority, EmptyOnInit) {
  PartialPriority D;
  D.initialize(4, u128(1000));
  EXPECT_TRUE(D.empty());
}

TEST(PartialPriority, InsertAndPullUpToM) {
  PartialPriority D;
  D.initialize(3, u128(1000));
  D.insert(1, u128(50));
  D.insert(2, u128(20));
  D.insert(3, u128(70));
  D.insert(4, u128(40));
  std::vector<int> out;
  unsigned __int128 B = 0;
  bool ok = D.pull(out, B);
  ASSERT_TRUE(ok);
  // Should return up to M=3 keys with smallest values: keys {2(20),4(40),1(50)}
  EXPECT_EQ(out.size(), 3u);
  // Boundary should be the max value among returned = 50
  EXPECT_EQ((uint64_t)B, 50u);
  // Next pull should return the remaining key 3
  out.clear();
  ok = D.pull(out, B);
  ASSERT_TRUE(ok);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0], 3);
  EXPECT_EQ((uint64_t)B, 70u);
  // Then empty
  out.clear();
  ok = D.pull(out, B);
  EXPECT_FALSE(ok);
  EXPECT_TRUE(D.empty());
}

TEST(PartialPriority, BatchPrependPartitionsAndOrders) {
  PartialPriority D;
  D.initialize(2, u128(1000)); // M=2, blocks should be size <= 2
  std::vector<PartialPriority::Item> batch = {
      {10, u128(30)}, {11, u128(10)}, {12, u128(20)}, {13, u128(40)}};
  D.batch_prepend(batch);
  // Pull first block (two smallest by value): keys {11(10),12(20)}, boundary=20
  std::vector<int> out;
  unsigned __int128 B = 0;
  bool ok = D.pull(out, B);
  ASSERT_TRUE(ok);
  ASSERT_EQ(out.size(), 2u);
  EXPECT_EQ((uint64_t)B, 20u);
  // Pull second block: {10(30),13(40)}, boundary=40
  out.clear();
  ok = D.pull(out, B);
  ASSERT_TRUE(ok);
  ASSERT_EQ(out.size(), 2u);
  EXPECT_EQ((uint64_t)B, 40u);
  // then empty
  out.clear();
  ok = D.pull(out, B);
  EXPECT_FALSE(ok);
}

TEST(PartialPriority, DecreaseKeyPreventsStaleReturns) {
  PartialPriority D;
  D.initialize(4, u128(1000));
  std::vector<PartialPriority::Item> batch = {{5, u128(80)}};
  D.batch_prepend(batch);
  // A worse insert should not change anything
  D.insert(5, u128(90));
  // A better insert should update and later stale entries are skipped
  D.insert(5, u128(30));
  std::vector<int> out;
  unsigned __int128 B = 0;
  bool ok = D.pull(out, B);
  ASSERT_TRUE(ok);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0], 5);
  EXPECT_EQ((uint64_t)B, 30u);
  // No more
  out.clear();
  ok = D.pull(out, B);
  EXPECT_FALSE(ok);
}
