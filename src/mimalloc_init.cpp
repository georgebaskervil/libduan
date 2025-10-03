#include <iostream>

#ifdef HAVE_MIMALLOC
#include <mimalloc.h>

// Initialize mimalloc with optimized settings for performance
// Based on configuration from RobusText project
__attribute__((constructor)) static void init_mimalloc() {
  // Allow large OS pages for better TLB performance
  mi_option_set(mi_option_allow_large_os_pages, 1);
  
  // Eager commit in arena for faster allocation
  mi_option_set(mi_option_arena_eager_commit, 1);
  
  // Use deprecated segment cache (can improve performance)
  mi_option_set(mi_option_deprecated_segment_cache, 1);
  
  // No delay in purging (immediate reclamation)
  mi_option_set(mi_option_purge_delay, 0);
  
  // Decommit pages when purging (return memory to OS)
  mi_option_set(mi_option_purge_decommits, 1);
  
  // Reserve huge OS pages for better performance
  mi_option_set(mi_option_reserve_huge_os_pages, 1);
  
  // Small delay for eager commit (2ms)
  mi_option_set(mi_option_eager_commit_delay, 2);
  
  // Use NUMA nodes for multi-socket systems
  mi_option_set(mi_option_use_numa_nodes, 1);

#ifdef ENABLE_MIMALLOC_DIAGNOSTICS
  // Show statistics at program exit (only when diagnostics enabled)
  mi_option_set(mi_option_show_stats, 1);
  mi_option_set(mi_option_show_errors, 1);
  mi_option_set(mi_option_verbose, 1);
  
  // Print mimalloc version
  std::cout << "mimalloc version: " << mi_version() << std::endl;
  std::cout << "mimalloc initialized with optimized settings" << std::endl;
#endif
}

#endif // HAVE_MIMALLOC
