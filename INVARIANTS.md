# BMSSP Algorithm Invariants

This document summarizes the key invariants maintained by the BMSSP algorithm implementation, as described in "Breaking the Sorting Barrier for Directed Single-Source Shortest Paths" (arXiv:2504.17033).

## Overview

The BMSSP algorithm maintains several critical invariants that ensure correctness and complexity bounds. These invariants are verified when `ENABLE_BMSSP_VERIFIER` is enabled.

## FindPivots Invariants (Algorithm 1, Lemma 3.5)

**Input**: Set S of source vertices, boundary B, parameter k

**Output**: Pivot set P ⊆ S, working set W

### Size Bounds

1. **|P| ≤ |W|/k** (after trimming if necessary)
   - Ensures pivot set is proportional to working set
   - Critical for amortized analysis
   - Checked in: `src/find_pivots.cpp` line ~265

2. **|W| ≤ k|S|** (unless early exit triggered)
   - Working set grows at most linearly with k
   - Early exit when |W| > k|S| returns P = S
   - Checked in: `src/find_pivots.cpp` lines ~165, ~266

### Layered Structure

3. **W = W_0 ∪ W_1 ∪ ... ∪ W_k** where W_0 = S
   - Each layer W_i contains vertices discovered in iteration i
   - Vertices added to W at most once
   - Implemented in: `src/find_pivots.cpp` lines ~120-165

4. **All vertices in W have dist < B**
   - Working set respects boundary constraint
   - Enforced during relaxation: `src/find_pivots.cpp` line ~145

### Forest Properties

5. **F is a shortest-path forest over tight edges in W**
   - Edge (u,v) ∈ F iff u,v ∈ W and dist[v] = dist[u] + w_{uv}
   - Used to compute subtree sizes for pivot selection
   - Built in: `src/find_pivots.cpp` lines ~175-200

6. **Each root in F is a vertex in S**
   - Roots are source vertices
   - Pivots selected from roots with large subtrees

## BaseCase Invariants (Algorithm 2)

**Input**: Single source x (complete), boundary B, parameter k

**Output**: Boundary B', vertex set U

### Preconditions

1. **|S| = 1** (single source)
   - Checked in: `src/base_case.cpp` line ~32

2. **Source x is complete** (dist[x] is optimal)
   - dist[x] should not change after BaseCase
   - Checked in: `src/base_case.cpp` line ~37

### Size Bounds

3. **|U| ≤ k**
   - Returns at most k vertices
   - If k+1 vertices discovered, boundary is tightened
   - Implemented in: `src/base_case.cpp` lines ~80-96

### Boundary Constraints

4. **All v ∈ U have dist[v] < B'**
   - Returned vertices respect the returned boundary
   - When |U| ≤ k: B' = B
   - When |U| = k+1: B' = max dist in first k vertices

### Completeness

5. **All v ∈ U are marked complete**
   - Complete vertices have optimal distances
   - Will not be relaxed further
   - Enforced in: `src/base_case.cpp` line ~117

## BMSSP Invariants (Algorithm 3, Lemma 3.9)

**Input**: Level l, boundary B, source set S

**Output**: Boundary B', vertex set U

### Termination Conditions

1. **Success case: B' = B and 𝒟 empty**
   - All vertices with dist < B have been processed
   - Checked in: `src/bmssp.cpp` line ~181

2. **Partial case: k·2^l·t ≤ |U| ≤ 4·k·2^l·t**
   - Triggered when U grows too large
   - Ensures bounded work per level
   - Checked in: `src/bmssp.cpp` lines ~223-230

### Disjointness

3. **U_i sets are disjoint across iterations**
   - Each vertex added to U at most once per BMSSP call
   - Critical for amortized analysis
   - Checked in: `src/bmssp.cpp` line ~131

### Boundary Monotonicity

4. **B_i' ≤ B_i ≤ B for all iterations i**
   - Boundaries are non-increasing
   - B_i is the pull boundary from 𝒟
   - B_i' is the recursive call's returned boundary

5. **All v ∈ U have dist[v] < B'**
   - Returned vertices respect final boundary
   - Checked in: `src/bmssp.cpp` line ~221

### Edge Insertion

6. **No edge (u,v) inserted twice into 𝒟 at same level**
   - Key assumption for amortized O(m) edge insertions
   - Tracked and verified when `ENABLE_BMSSP_VERIFIER` is on
   - Checked in: `src/bmssp.cpp` lines ~158-163

### Completeness

7. **All v ∈ U are marked complete**
   - Complete vertices have optimal distances (within boundary)
   - Enforced in: `src/bmssp.cpp` line ~137

## Distance State Invariants

### Widening Invariants

1. **Active width is monotone non-decreasing**
   - Once widened, never narrow
   - 32 → 64 → 128 → BigInt

2. **Complete vertices unchanged after widening**
   - Widening affects only incomplete vertices
   - Critical for correctness
   - Checked in: `tests/boundary_invariants_test.cpp`

### Lexicographic Ordering

3. **Tie-breaking: (dist, hop_count, vertex_id)**
   - When distances equal, prefer fewer hops
   - When hops equal, prefer lower vertex ID
   - Implemented in: `src/bmssp_state.cpp` lines ~200-230

## Data Structure 𝒟 Invariants (Lemma 3.3)

**Operations**: Initialize(M, B), Insert(key, value), BatchPrepend(items), Pull()

### Capacity

1. **Pull returns at most M items**
   - M = 2^{l-1} · t for level l
   - Enforced in: `src/structure.cpp` line ~88

### Boundary

2. **All returned items have value < B**
   - B is the global boundary
   - Enforced during initialization

### Decrease-Key Semantics

3. **Only improvements update stored values**
   - If key exists with lower value, ignore new insertion
   - Maintains monotonicity of returned batches
   - Implemented in: `src/structure.cpp` lines ~43-48

## Verification

All invariants marked with "Checked in" are verified when `ENABLE_BMSSP_VERIFIER` is enabled at compile time:

```zsh
cmake --preset debug -DENABLE_BMSSP_VERIFIER=ON
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Failed assertions indicate violations of algorithm invariants and should be investigated.

## References

- Paper: "Breaking the Sorting Barrier for Directed Single-Source Shortest Paths"
- Algorithm 1 (FindPivots): Paper Section 3, Lemma 3.5
- Algorithm 2 (BaseCase): Paper Section 3
- Algorithm 3 (BMSSP): Paper Section 3, Lemma 3.9
- Data Structure 𝒟: Paper Lemma 3.3
