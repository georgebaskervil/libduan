#include <gtest/gtest.h>
#include "cpp_starter/bmssp/degree_check.hpp"
#include "cpp_starter/bmssp/graph.hpp"
#include "cpp_starter/bmssp/bmssp.hpp"

using namespace bmssp;

TEST(DegreeCheck, ComputeMaxDegrees) {
  // Graph with known degrees
  //  0 -> 1, 2, 3  (out-degree 3)
  //  1 -> 2        (out-degree 1)
  //  2 -> 3        (out-degree 1)
  // Vertex 3 has in-degree 2
  Graph g(4);
  g.add_edge(0, 1, 1);
  g.add_edge(0, 2, 1);
  g.add_edge(0, 3, 1);
  g.add_edge(1, 2, 1);
  g.add_edge(2, 3, 1);
  
  auto [max_out, max_in] = compute_max_degrees(g);
  
  EXPECT_EQ(max_out, 3u);  // Vertex 0 has out-degree 3
  EXPECT_EQ(max_in, 2u);   // Vertex 3 has in-degree 2
}

TEST(DegreeCheck, LowDegreeGraphPassesValidation) {
  // Graph with all degrees ≤ 2
  Graph g(4);
  g.add_edge(0, 1, 1);
  g.add_edge(0, 2, 1);  // Vertex 0 has out-degree 2
  g.add_edge(1, 3, 1);
  g.add_edge(2, 3, 1);  // Vertex 3 has in-degree 2
  
  // Should not abort or warn (degree ≤ 2)
  validate_degree_constraint(g);
  
  auto [max_out, max_in] = compute_max_degrees(g);
  EXPECT_LE(max_out, 2u);
  EXPECT_LE(max_in, 2u);
}

TEST(DegreeCheck, HighDegreeGraphWarnsWhenVerifierOn) {
  // Graph with degree > 2
  Graph g(5);
  g.add_edge(0, 1, 1);
  g.add_edge(0, 2, 1);
  g.add_edge(0, 3, 1);
  g.add_edge(0, 4, 1);  // Vertex 0 has out-degree 4 > 2
  
  auto [max_out, max_in] = compute_max_degrees(g);
  EXPECT_GT(max_out, 2u);
  
#ifdef ENABLE_BMSSP_VERIFIER
  std::cout << "\nExpected warning about degree > 2:\n";
  // This will warn but not abort (unless CONST_DEGREE_STRICT_MODE is on)
  validate_degree_constraint(g);
  std::cout << "Warning printed successfully.\n";
#else
  // No warning when verifier is off
  validate_degree_constraint(g);
#endif
}

TEST(DegreeCheck, RunSSspOnLowDegreeGraph) {
  // Ensure run_sssp works on low-degree graphs
  Graph g(4);
  g.add_edge(0, 1, 5);
  g.add_edge(1, 2, 5);
  g.add_edge(2, 3, 5);
  
  auto dist = run_sssp(g, 0);
  
  EXPECT_EQ(dist[0], 0u);
  EXPECT_EQ(dist[1], 5u);
  EXPECT_EQ(dist[2], 10u);
  EXPECT_EQ(dist[3], 15u);
}
