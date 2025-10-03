# Duan's BMSSP Algorithm Implementation

A faithful C++20 implementation of the **Breaking the Sorting Barrier** algorithm for computing single-source shortest paths (SSSP) in directed graphs with non-negative integer weights, from the paper:

> **"Breaking the Sorting Barrier for Directed Single-Source Shortest Paths"**  
> arXiv:2504.17033

This implementation achieves **O(m + n log^(2/3) n)** time complexity for constant-degree graphs, breaking the O(m + n log n) sorting barrier imposed by traditional Dijkstra-based approaches.

## Algorithm Overview

The BMSSP (Bounded Multi-Source Shortest Path) algorithm uses:

- **Layered relaxation** (FindPivots) to identify pivot vertices with large subtrees
- **Recursive decomposition** with a priority queue data structure 𝒟 to batch-process vertices
- **Base case** mini-Dijkstra for small subproblems
- **Automatic parameter tuning**: k = ⌊log^(1/3) n⌋, t = ⌊log^(2/3) n⌋, l_max = ⌈log n / t⌉

### Key Features

✅ **Faithful paper implementation**: Algorithms 1-3 (FindPivots, BaseCase, BMSSP)  
✅ **Automatic parameter computation**: No manual tuning required  
✅ **Distance widening**: 32→64→128→BigInt on overflow  
✅ **Comprehensive testing**: 43 tests covering correctness, edge cases, and invariants  
✅ **Performance instrumentation**: Track relax operations, amortized costs, edge insertions  
✅ **Degree validation**: Warn when input violates constant-degree assumption  

## Quick Start

### Requirements

- macOS with Xcode command line tools (clang, make) or Linux with GCC/Clang
- CMake >= 3.20
- Optional: mimalloc (macOS: `brew install mimalloc`, Ubuntu: `sudo apt-get install libmimalloc-dev`)

### Build and Test

```zsh
# Configure (Debug with verifier enabled)
cmake --preset debug -DENABLE_BMSSP_VERIFIER=ON

# Build
cmake --build --preset debug

# Run tests (43 tests)
ctest --preset debug --output-on-failure

# Run the example application
./build/debug/bin/libduan
```

### Basic Usage

```cpp
#include "cpp_starter/bmssp/bmssp.hpp"
#include "cpp_starter/bmssp/graph.hpp"

using namespace bmssp;

// Create a directed graph
Graph g(5);
g.add_edge(0, 1, 10);
g.add_edge(1, 2, 20);
g.add_edge(0, 3, 5);
g.add_edge(3, 2, 15);

// Run SSSP from source 0
std::vector<uint64_t> dist = run_sssp(g, 0);

// dist[0] = 0, dist[1] = 10, dist[2] = 20 (via 3), dist[3] = 5
```

## Build Configuration

### Memory Allocator Options

- **`ENABLE_MIMALLOC`** (default: ON)  
  Use mimalloc high-performance allocator if available.  
  Provides better performance than system allocator for graph algorithms.

- **`ENABLE_GLOBAL_MIMALLOC`** (default: OFF)  
  Override global new/delete operators to use mimalloc.  
  Applies mimalloc to all C++ allocations including STL containers.

- **`ENABLE_MIMALLOC_DIAGNOSTICS`** (default: OFF)  
  Print mimalloc version and statistics at program startup.  
  Useful for verifying mimalloc configuration.

**Optimized mimalloc settings** (automatically applied):
- Large OS pages enabled for better TLB performance
- Eager arena commit for faster allocation
- NUMA-aware allocation for multi-socket systems
- Zero purge delay for immediate memory reclamation
- Huge OS page reservation when available

### Core Algorithm Options

- **`ENABLE_BMSSP_VERIFIER`** (default: OFF)  
  Enable invariant checking, statistics collection, and degree validation.  
  Recommended for development and debugging.

- **`CONST_DEGREE_STRICT_MODE`** (default: OFF)  
  Abort if input graph has degree > 2 (requires `ENABLE_BMSSP_VERIFIER`).  
  Useful for testing that inputs satisfy the paper's constant-degree assumption.

### Distance Representation Options

- **`MIN_DISTANCE_BITS`** (default: 64)  
  Initial distance width: 32 or 64 bits.

- **`ENABLE_DISTANCE_WIDENING`** (default: ON)  
  Automatically widen distances on overflow: 32→64→128 bits.  
  If OFF, overflow throws an exception.

- **`ENABLE_BIGINT_FALLBACK`** (default: OFF)  
  Promote beyond 128-bit to arbitrary-precision BigInt on overflow.  
  Experimental feature for extreme edge weights.

### Examples

```zsh
# Release build with verifier
cmake --preset release -DENABLE_BMSSP_VERIFIER=ON
cmake --build --preset release

# Strict mode (abort on degree > 2)
cmake --preset debug -DENABLE_BMSSP_VERIFIER=ON -DCONST_DEGREE_STRICT_MODE=ON

# BigInt fallback enabled
cmake --preset debug -DENABLE_BIGINT_FALLBACK=ON
```

## Algorithm Parameters

The implementation auto-computes optimal parameters based on graph size n:

| Parameter | Formula | Description |
|-----------|---------|-------------|
| k | ⌊log^(1/3) n⌋ | Pivot threshold (≥1) |
| t | ⌊log^(2/3) n⌋ | Batch size factor (≥1) |
| l_max | ⌈log n / t⌉ | Maximum recursion depth |
| M | 2^(l-1) · t | Data structure capacity at level l |

These parameters achieve the O(m + n log^(2/3) n) complexity for constant-degree graphs.

## Complexity and Limitations

### Time Complexity

- **O(m + n log^(2/3) n)** for constant-degree graphs (degree ≤ 2)
- **O(m log n + n log^(2/3) n)** for general graphs

The constant-degree assumption is crucial for the optimal bound. The implementation validates this assumption when `ENABLE_BMSSP_VERIFIER` is enabled.

### Space Complexity

- **O(n)** for distance arrays and auxiliary structures
- Additional O(m) for edge relaxation tracking (verifier only)

### Limitations

1. **Integer weights only**: Non-negative integer edge weights required
2. **Constant degree**: Optimal O(m + n log^(2/3) n) bound assumes degree ≤ 2
3. **Dense parameter tuning**: Performance sensitive to k, t, l_max choices
4. **Practical overhead**: May be slower than Dijkstra for small graphs (n < 10,000)

## Testing

The implementation includes 43 comprehensive tests:

- **Correctness**: Validated against reference Dijkstra implementation
- **Algorithm components**: FindPivots, BaseCase, BMSSP recursion
- **Edge cases**: Empty graphs, disconnected vertices, single vertex
- **Invariants**: Size bounds, disjointness, partial termination
- **Distance widening**: Overflow handling, BigInt promotion
- **Tie-breaking**: Lexicographic ordering on equal distances
- **Degree checking**: Validation of constant-degree assumption

Run tests with:

```zsh
ctest --preset debug --output-on-failure
```

## Performance Instrumentation

When `ENABLE_BMSSP_VERIFIER` is enabled, the implementation tracks:

- **Relaxation operations**: attempts, improvements, equal-distance accepts
- **Distance widening**: overflow events, bytes widened, BigInt usage
- **Amortized metrics**: relax per edge, improvements per vertex
- **Edge tracking**: Validates no edge inserted twice (key assumption)

## Project Structure

```text
include/cpp_starter/bmssp/
  ├── bmssp.hpp           # Top-level SSSP driver
  ├── find_pivots.hpp     # Algorithm 1: FindPivots
  ├── base_case.hpp       # Algorithm 2: BaseCase
  ├── graph.hpp           # Graph representation
  ├── state.hpp           # Distance state & widening
  ├── structure.hpp       # Priority queue 𝒟 (Lemma 3.3)
  ├── degree_check.hpp    # Degree validation
  └── instrumentation.hpp # Performance stats

src/
  ├── bmssp.cpp           # Algorithm 3: BMSSP recursion
  ├── find_pivots.cpp     # Layered relaxation & pivot selection
  ├── base_case.cpp       # Mini-Dijkstra for base case
  ├── bmssp_state.cpp     # Distance operations & relax
  ├── structure.cpp       # Data structure 𝒟 implementation
  ├── degree_check.cpp    # Degree computation & validation
  └── instrumentation.cpp # Stats dumping

tests/                    # 43 comprehensive tests
```

## References

- Paper: "Breaking the Sorting Barrier for Directed Single-Source Shortest Paths" (arXiv:2504.17033)
- Algorithms: FindPivots (Alg 1), BaseCase (Alg 2), BMSSP (Alg 3)
- Data Structure: Lemma 3.3 (bounded priority queue 𝒟)

## License

> Placeholder comment only. Add your own license text if needed.

Example (pseudo-usage inside your driver):

```cpp
DistState st = make_state(n);
st.fastpath_u64_max_sum = static_cast<uint64_t>((n-1)) * static_cast<uint64_t>(max_edge_w);
```

Running the widening benchmark (counts only)

```zsh
# Configure with verifier enabled to see counters
cmake --preset debug -DENABLE_BMSSP_VERIFIER=ON
cmake --build --preset debug
ctest --preset debug --output-on-failure -V -R BMSSPWidenBench
```

Trade-offs

- Reliability: With widening and BigInt, distances never overflow for non-negative weights; memory and CPU cost increase when widening triggers, especially with BigInt.
- Performance: The fast-path improves hot-loop performance when a safe bound is known. If the bound is too high (effectively max uint64), most operations still take the branchless sum path; near-bound additions may still require checks or widening.
- Determinism: All arithmetic is integer-based; results are exact and deterministic across platforms given identical flags.
