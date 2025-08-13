// License: Placeholder - add your license details here.
#include <gtest/gtest.h>
#include "cpp_starter/lib.hpp"

TEST(CppStarter, SumBasics) {
  EXPECT_EQ(cpp_starter::sum(0, 0), 0);
  EXPECT_EQ(cpp_starter::sum(1, 2), 3);
  EXPECT_EQ(cpp_starter::sum(-5, 5), 0);
}

TEST(CppStarter, SumEdge) {
  EXPECT_EQ(cpp_starter::sum(2147483640, 5), 2147483645);
}
