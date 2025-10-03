#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>

#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"

using namespace bmssp;

namespace {

// Simple Dijkstra implementation for comparison
std::vector<uint64_t> dijkstra_baseline(const Graph& g, int s) {
  const std::size_t n = g.size();
  const uint64_t INF = std::numeric_limits<uint64_t>::max() / 4;
  std::vector<uint64_t> dist(n, INF);
  std::vector<bool> visited(n, false);
  
  if (s < 0 || static_cast<std::size_t>(s) >= n) return dist;
  
  dist[static_cast<std::size_t>(s)] = 0;
  
  // Simple O(n^2) Dijkstra
  for (std::size_t iter = 0; iter < n; ++iter) {
    int u = -1;
    uint64_t best = INF;
    for (std::size_t i = 0; i < n; ++i) {
      if (!visited[i] && dist[i] < best) {
        best = dist[i];
        u = static_cast<int>(i);
      }
    }
    
    if (u == -1) break;
    visited[static_cast<std::size_t>(u)] = true;
    
    for (const auto& e : g.neighbors(u)) {
      if (e.to < 0 || static_cast<std::size_t>(e.to) >= n) continue;
      uint64_t cand = dist[static_cast<std::size_t>(u)] + e.w;
      if (cand < dist[static_cast<std::size_t>(e.to)]) {
        dist[static_cast<std::size_t>(e.to)] = cand;
      }
    }
  }
  
  return dist;
}

struct BenchResult {
  std::size_t n;
  std::size_t m;
  double dijkstra_ms;
  double bmssp_ms;
  double speedup;
  bool correct;
};

void print_results(const std::vector<BenchResult>& results) {
  std::cout << "\n=== Performance Comparison: BMSSP vs Dijkstra ===" << std::endl;
  std::cout << std::string(80, '=') << std::endl;
  std::cout << std::left << std::setw(12) << "Graph Size"
            << std::setw(12) << "Edges"
            << std::setw(15) << "Dijkstra (ms)"
            << std::setw(15) << "BMSSP (ms)"
            << std::setw(12) << "Speedup"
            << std::setw(10) << "Correct" << std::endl;
  std::cout << std::string(80, '-') << std::endl;
  
  for (const auto& r : results) {
    std::cout << std::left << std::setw(12) << r.n
              << std::setw(12) << r.m
              << std::setw(15) << std::fixed << std::setprecision(3) << r.dijkstra_ms
              << std::setw(15) << std::fixed << std::setprecision(3) << r.bmssp_ms
              << std::setw(12) << std::fixed << std::setprecision(2) << r.speedup << "x"
              << std::setw(10) << (r.correct ? "✓" : "✗") << std::endl;
  }
  std::cout << std::string(80, '=') << std::endl;
}

Graph generate_random_graph(std::size_t n, int max_degree, int seed) {
  Graph g(n);
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> deg_dist(1, max_degree);
  std::uniform_int_distribution<int> v_dist(0, static_cast<int>(n) - 1);
  std::uniform_int_distribution<uint64_t> w_dist(1, 100);
  
  for (std::size_t u = 0; u < n; ++u) {
    int degree = deg_dist(rng);
    for (int i = 0; i < degree; ++i) {
      int v = v_dist(rng);
      if (v == static_cast<int>(u)) continue;
      g.add_edge(static_cast<int>(u), v, w_dist(rng));
    }
  }
  
  return g;
}

std::size_t count_edges(const Graph& g) {
  std::size_t m = 0;
  for (std::size_t u = 0; u < g.size(); ++u) {
    m += g.neighbors(static_cast<int>(u)).size();
  }
  return m;
}

bool verify_distances(const std::vector<uint64_t>& dijkstra, 
                     const std::vector<uint64_t>& bmssp,
                     std::size_t n) {
  if (dijkstra.size() != n || bmssp.size() != n) return false;
  for (std::size_t i = 0; i < n; ++i) {
    if (dijkstra[i] != bmssp[i]) {
      return false;
    }
  }
  return true;
}

} // namespace

TEST(BMSSPPerformance, CompareAgainstDijkstraSmallGraphs) {
  std::vector<BenchResult> results;
  std::vector<std::size_t> sizes = {100, 250, 500, 1000};
  const int max_degree = 2; // Constant degree for optimal BMSSP performance
  const int source = 0;
  const int seed = 42;
  
  for (std::size_t n : sizes) {
    Graph g = generate_random_graph(n, max_degree, seed + static_cast<int>(n));
    std::size_t m = count_edges(g);
    
    // Warm up
    auto warm = run_sssp(g, source);
    (void)warm;
    
    // Benchmark Dijkstra
    auto start_dij = std::chrono::high_resolution_clock::now();
    auto dijkstra_result = dijkstra_baseline(g, source);
    auto end_dij = std::chrono::high_resolution_clock::now();
    double dijkstra_ms = std::chrono::duration<double, std::milli>(end_dij - start_dij).count();
    
    // Benchmark BMSSP
    auto start_bmssp = std::chrono::high_resolution_clock::now();
    auto bmssp_result = run_sssp(g, source);
    auto end_bmssp = std::chrono::high_resolution_clock::now();
    double bmssp_ms = std::chrono::duration<double, std::milli>(end_bmssp - start_bmssp).count();
    
    // Verify correctness
    bool correct = verify_distances(dijkstra_result, bmssp_result, n);
    EXPECT_TRUE(correct) << "Results differ for n=" << n;
    
    // Calculate speedup
    double speedup = dijkstra_ms / bmssp_ms;
    
    results.push_back({n, m, dijkstra_ms, bmssp_ms, speedup, correct});
  }
  
  print_results(results);
  
  // Check that we're getting reasonable performance
  // (not necessarily faster on small graphs due to overhead)
  bool all_correct = true;
  for (const auto& r : results) {
    all_correct = all_correct && r.correct;
  }
  EXPECT_TRUE(all_correct);
}

TEST(BMSSPPerformance, CompareAgainstDijkstraMediumGraphs) {
  std::vector<BenchResult> results;
  std::vector<std::size_t> sizes = {2000, 4000, 8000};
  const int max_degree = 2; // Constant degree for optimal BMSSP performance
  const int source = 0;
  const int seed = 123;
  
  for (std::size_t n : sizes) {
    Graph g = generate_random_graph(n, max_degree, seed + static_cast<int>(n));
    std::size_t m = count_edges(g);
    
    std::cout << "\nBenchmarking n=" << n << ", m=" << m << "..." << std::endl;
    
    // Benchmark Dijkstra
    auto start_dij = std::chrono::high_resolution_clock::now();
    auto dijkstra_result = dijkstra_baseline(g, source);
    auto end_dij = std::chrono::high_resolution_clock::now();
    double dijkstra_ms = std::chrono::duration<double, std::milli>(end_dij - start_dij).count();
    
    // Benchmark BMSSP
    auto start_bmssp = std::chrono::high_resolution_clock::now();
    auto bmssp_result = run_sssp(g, source);
    auto end_bmssp = std::chrono::high_resolution_clock::now();
    double bmssp_ms = std::chrono::duration<double, std::milli>(end_bmssp - start_bmssp).count();
    
    // Verify correctness
    bool correct = verify_distances(dijkstra_result, bmssp_result, n);
    EXPECT_TRUE(correct) << "Results differ for n=" << n;
    
    // Calculate speedup
    double speedup = dijkstra_ms / bmssp_ms;
    
    results.push_back({n, m, dijkstra_ms, bmssp_ms, speedup, correct});
  }
  
  print_results(results);
  
  // For medium-sized graphs, BMSSP should show advantage
  // At n=8000, we expect competitive or better performance
  bool all_correct = true;
  for (const auto& r : results) {
    all_correct = all_correct && r.correct;
  }
  EXPECT_TRUE(all_correct);
}

TEST(BMSSPPerformance, ScalabilityAnalysis) {
  std::cout << "\n=== Scalability Analysis ===" << std::endl;
  std::cout << "Testing O(m + n log^(2/3) n) vs O(n^2) growth" << std::endl;
  
  std::vector<BenchResult> results;
  std::vector<std::size_t> sizes = {1000, 2000, 4000, 8000};
  const int max_degree = 2;
  const int source = 0;
  const int seed = 456;
  
  for (std::size_t n : sizes) {
    Graph g = generate_random_graph(n, max_degree, seed + static_cast<int>(n));
    std::size_t m = count_edges(g);
    
    // Benchmark BMSSP only (Dijkstra gets too slow)
    auto start = std::chrono::high_resolution_clock::now();
    auto bmssp_result = run_sssp(g, source);
    auto end = std::chrono::high_resolution_clock::now();
    double bmssp_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Theoretical complexity (for comparison)
    double n_squared = static_cast<double>(n * n);
    double n_log_2_3 = static_cast<double>(n) * std::pow(std::log(static_cast<double>(n)), 2.0/3.0);
    
    std::cout << "n=" << std::setw(6) << n 
              << " | m=" << std::setw(6) << m
              << " | BMSSP: " << std::setw(8) << std::fixed << std::setprecision(2) << bmssp_ms << " ms"
              << " | n²: " << std::setw(12) << std::fixed << std::setprecision(0) << n_squared
              << " | n·log^(2/3)n: " << std::setw(12) << std::fixed << std::setprecision(0) << n_log_2_3
              << std::endl;
    
    results.push_back({n, m, 0.0, bmssp_ms, 0.0, true});
  }
  
  // Verify sub-quadratic scaling
  if (results.size() >= 2) {
    double growth = results.back().bmssp_ms / results.front().bmssp_ms;
    double size_ratio = static_cast<double>(results.back().n) / static_cast<double>(results.front().n);
    double quadratic_growth = size_ratio * size_ratio;
    
    std::cout << "\nGrowth analysis (n=" << results.front().n << " to n=" << results.back().n << "):" << std::endl;
    std::cout << "  Size ratio: " << std::fixed << std::setprecision(2) << size_ratio << "x" << std::endl;
    std::cout << "  Actual time growth: " << std::fixed << std::setprecision(2) << growth << "x" << std::endl;
    std::cout << "  Quadratic (O(n²)) would be: " << std::fixed << std::setprecision(2) << quadratic_growth << "x" << std::endl;
    std::cout << "  BMSSP scaling is " << std::fixed << std::setprecision(2) 
              << (quadratic_growth / growth) << "x better than quadratic!" << std::endl;
    
    // BMSSP should scale much better than O(n²)
    EXPECT_LT(growth, quadratic_growth * 0.8) << "BMSSP should scale better than O(n²)";
  }
}
