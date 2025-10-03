#include "cpp_starter/bmssp/state.hpp"
#include <iostream>
#include <iomanip>

namespace bmssp {

#ifdef ENABLE_BMSSP_VERIFIER

void dump_stats(const DistState& st, std::size_t n_vertices, std::size_t n_edges) {
  std::cout << "\n=== BMSSP Performance Statistics ===\n";
  
  // Relaxation stats
  std::cout << "\nRelaxation Operations:\n";
  std::cout << "  Total attempts:     " << st.relax_attempts << "\n";
  std::cout << "  Improvements:       " << st.relax_improve_events << "\n";
  std::cout << "  Equal (accepted):   " << st.relax_equal_events << "\n";
  std::cout << "  Rejected:           " << st.relax_reject_events << "\n";
  
  if (n_edges > 0) {
    double relax_per_edge = static_cast<double>(st.relax_attempts) / n_edges;
    std::cout << "  Relax per edge:     " << std::fixed << std::setprecision(2) 
              << relax_per_edge << "\n";
  }
  
  // Widening stats
  if (st.widen_events > 0 || st.used_bigint) {
    std::cout << "\nDistance Widening:\n";
    std::cout << "  Widen events:       " << st.widen_events << "\n";
    std::cout << "  Overflow events:    " << st.overflow_events << "\n";
    std::cout << "  Widen pairs:        " << st.widen_pairs << "\n";
    std::cout << "  Bytes widened:      " << st.widen_bytes << "\n";
    
    if (st.used_bigint) {
      std::cout << "  BigInt used:        YES\n";
      std::cout << "  BigInt limbs:       " << st.bigint_total_limbs << "\n";
      std::cout << "  BigInt bytes:       " << st.big_bytes << "\n";
    }
  }
  
  // Simulated failures
  if (st.oom_simulated_events > 0) {
    std::cout << "\nSimulated Events:\n";
    std::cout << "  OOM simulations:    " << st.oom_simulated_events << "\n";
  }
  
  // Amortized metrics
  std::cout << "\nAmortized Metrics:\n";
  if (n_vertices > 0) {
    double improve_per_vertex = static_cast<double>(st.relax_improve_events) / n_vertices;
    std::cout << "  Improvements/vertex: " << std::fixed << std::setprecision(2)
              << improve_per_vertex << "\n";
  }
  
  std::cout << "\n====================================\n";
}

#else

void dump_stats(const DistState&, std::size_t, std::size_t) {
  std::cout << "Statistics disabled (ENABLE_BMSSP_VERIFIER not defined)\n";
}

#endif

}  // namespace bmssp
