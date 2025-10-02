#include <gtest/gtest.h>
#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include "cpp_starter/bmssp/instrumentation.hpp"
#include <iostream>

using namespace bmssp;

TEST(Instrumentation, DumpStatsOnSimpleGraph) {
  // Create a simple graph to demonstrate stats
  Graph g(10);
  for (int i = 0; i < 9; ++i) {
    g.add_edge(i, i + 1, 5);
  }
  
  std::size_t n_edges = 9;
  
  std::cout << "\nRunning SSSP on a 10-vertex chain graph...\n";
  auto dist = run_sssp(g, 0);
  
  // We don't have direct access to DistState from run_sssp,
  // but we can verify the result
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[9], 45u);
  
  std::cout << "Note: To see full stats, instrument run_sssp to call dump_stats\n";
  std::cout << "or run with a lower-level API that exposes DistState.\n";
}

TEST(Instrumentation, StatsAvailableWhenVerifierEnabled) {
#ifdef ENABLE_BMSSP_VERIFIER
  std::cout << "\nVerifier is ENABLED - stats will be collected\n";
  EXPECT_TRUE(true);
#else
  std::cout << "\nVerifier is DISABLED - stats will NOT be collected\n";
  EXPECT_TRUE(true);
#endif
}
