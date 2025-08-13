# BMSSP Implementation TODO

Goal: Replace prototype BMSSP code with a faithful implementation of Algorithms 1–3 (FindPivots, BaseCase, BMSSP) and Lemma 3.3 data structure from the paper "Breaking the Sorting Barrier for Directed Single-Source Shortest Paths" (arXiv:2504.17033).

---

## Legend

- [ ] = not started
- [~] = in progress
- [x] = done
- (opt) = optional / stretch
- PRI: H=High, M=Medium, L=Low

---

## Phase 0: Baseline & Goals

- [x] Capture current prototype snapshot (tag or branch) before refactor. (Safety commit already made.)
- [x] Define success criteria: passes correctness tests vs Boost Graph Library Dijkstra (Boost REQUIRED) on random constant-degree graphs; invariants hold (|P| ≤ |W|/k, etc.).
  - [x] Require Boost (latest stable) via `find_package(Boost REQUIRED)`; fail CMake if not found (no fallback gating tests).
  - [x] Exact distance equality required (integer widening ensures determinism).
  - [x] Random test parameters: sizes {64, 256, 1024, 4096}; per size 10 graphs; uniform integer weights in [1,100]; out-degree ≤ 2.
  - [x] Reproducible seeds: first 10 primes {2,3,5,7,11,13,17,19,23,29}.
  - [x] Default fail-fast behavior on first mismatch; optional aggregation flag.
  - [x] Configuration options to add (later phases):
      * `BMSSP_FAIL_FAST_TESTS` (ON default) – when OFF, aggregate & report all mismatches.
      * `BMSSP_REQUIRE_EXACT_VERIFY` (ON default) – guard to prevent approximate modes.
      * `BMSSP_AGGREGATE_TEST_ERRORS` (OFF default) alias / secondary toggle (mutually exclusive with fail-fast when ON).
  - [x] Internal simple Dijkstra retained only for micro-bench / diagnostic (never authoritative for pass/fail when Boost present).
- [x] Decide numeric types & precision handling strategy: integer distances with widening + optional bigint fallback (see Phase 2 / 2b).

## Phase 1: Module Scaffolding (H)

- [ ] Create directory `include/cpp_starter/bmssp/` & `src/bmssp/`.
- [ ] Split interfaces:
  - graph.hpp
  - state.hpp (DistanceState / Relaxation API)
  - structure.hpp (Data structure 𝒟 interface per Lemma 3.3)
  - find_pivots.hpp / .cpp
  - base_case.hpp / .cpp
  - bmssp.hpp / .cpp (recursive engine + top level driver)
  - verify.hpp (instrumentation/asserts)
- [ ] Move prototype BMSSP code to `src/legacy/bmssp_prototype.cpp` (keep for reference; exclude from build by default).
- [ ] Update CMake to add new sources & optional feature flag `ENABLE_BMSSP_VERIFIER`.

## Phase 2: Distance / State Representation (H)

- [ ] Implement `DistanceState` containing:
  - dist[]
  - pred[]
  - hop_count[] (for tie-breaking under Assumption 2.1 surrogate)
  - complete[] (bool)
- [ ] Provide `relax(u,v,w,allow_equal=true)` implementing ≤ rule (Remark 3.4) and predecessor update logic with lexicographic tie-breaking: (dist, hop_count, vertex_id).
- [ ] Add function to mark vertex complete.
- [ ] Add initialization helpers from source set S.
- [ ] Adopt integer distances with overflow detection (non-negative weights). Initial underlying width = configurable `MIN_DISTANCE_BITS` (default 64; allow 32 for small graphs).
- [ ] Implement widening `DistWord`: 32 -> 64 -> 128 (via built-in unsigned __int128 if available) -> big (vector<uint64_t>) as last resort (guarded by option).
- [ ] Overflow detection using compiler intrinsics (e.g., `__builtin_add_overflow`) with portable fallback; on overflow trigger widening & full dist array reallocation.
- [ ] CMake options:
  - `MIN_DISTANCE_BITS` (32|64) default 64.
  - `ENABLE_DISTANCE_WIDENING` (ON default) to attempt widening before signaling fatal error.
  - `ENABLE_BIGINT_FALLBACK` (OFF default) enabling final arbitrary precision layer.
- [ ] Logging (verifier mode): record widen events & memory usage delta.
- [ ] Failure semantics: If widening disabled or OOM during widen, raise fatal error (consistent with eventual OOM consequence).

## Phase 2b: Distance Scaling & Reliability (M)

- [ ] Stress tests generating path lengths near 2^32 and 2^64 boundaries.
- [ ] Invariant: distances of previously complete vertices identical post-widen.
- [ ] Benchmark: widening enabled vs disabled overhead (% extra time, memory).
- [ ] Fast path optimization when (n-1)*max_edge_weight fits in initial width (skip overflow checks in hot loop via constexpr flag).
- [ ] Document reliability vs resource trade-off in README.

## Phase 3: Faithful FindPivots (Algorithm 1) (H)

- [ ] Implement layered relaxation for exactly k iterations (or early termination) with bound B.
- [ ] Maintain W_i layers; union into W.
- [ ] Early exit condition: if |W| > k|S| then return P=S, W current.
- [ ] Build forest F: edges (u,v) where u,v ∈ W and dist[v] == dist[u] + w_uv after final relaxation.
- [ ] Compute subtree sizes in F for each root in S; mark pivots where subtree size ≥ k.
- [ ] Return (P,W); ensure P ⊆ S, |P| ≤ |W|/k.
- [ ] Instrumentation asserts: |W| ≤ min(k|S|, |U~|) when no early exit.

## Phase 4: Data Structure 𝒟 (Lemma 3.3) (H)

- [ ] Design block structures:  
  - Each block should contain:
    - A `vector` (or similar sequence) of elements.
    - An `upper_bound` value representing the maximum key in the block.
- [ ] Two sequences: D0 (batch prepends) + D1 (inserts) with std::map keyed by block upper bound for D1 ordering.
- [ ] Operations:
  - Initialize(M, B)
  - Insert(key,value) (update if lower)
  - BatchPrepend(list\<key,value\>) (partition into blocks ≤ M using median selection / nth_element)
  - Pull() -> (S_i (≤ M keys), B_i boundary)
  - Delete internal (used in Pull)
- [ ] Support complexity heuristics; add counters for amortized analysis (instrumentation only).
- [ ] Provide fallback simple structure behind a compile flag for debugging.

## Phase 5: BaseCase (Algorithm 2) (M)

- [ ] Preconditions: |S| == 1, source x is complete.
- [ ] Mini Dijkstra limited by bound B; heap keyed by dist.
- [ ] Stop when heap empty OR collected k+1 vertices.
- [ ] If collected ≤ k: B' = B; U = U0.
- [ ] Else B' = max dist in U0; U = {v ∈ U0 | dist[v] < B'}.
- [ ] Mark all U vertices complete.
- [ ] Assert invariants.

## Phase 6: Recursive BMSSP (Algorithm 3) (H)

- [ ] Implement procedure BMSSP(l,B,S):
  - If l==0 -> BaseCase.
  - Run FindPivots -> (P,W).
  - Initialize 𝒟 with M = 2^{l-1} * t, B.
  - Insert each x∈P with dist[x].
  - Set B0' = (P empty ? B : min dist[x]).
  - Loop iterations:
    1. Pull -> (S_i, B_i)
    2. Recursive call BMSSP(l-1, B_i, S_i) -> (B_i', U_i)
    3. Add U_i into U (track disjointness) and mark complete.
    4. Relax edges from U_i (≤ condition). For each relaxation:
       - If dist in [B_i, B): Insert
       - Else if in [B_i', B_i): add to K
    5. BatchPrepend K plus qualifying S_i vertices (dist in [B_i', B_i)).
    6. If 𝒟 empty -> success; B' = B; break.
    7. If |U| ≥ k *2^l* t -> partial; B' = B_i'; break.
  - After loop add W' = {x∈W | dist[x] < B'} to U.
  - Assert: success vs partial conditions & size bounds (Lemma 3.9).
- [ ] Return (B', U).

## Phase 7: Top-Level SSSP Driver (M)

- [ ] Auto-compute parameters:
  - k = floor(log^{1/3} n) (≥1)
  - t = floor(log^{2/3} n) (≥1)
  - l_max = ceil(log n / t)
- [ ] Initialize S = {s}, B = ∞.
- [ ] Call BMSSP(l_max, B, S).
- [ ] Expose API: `run_sssp(const Graph&, int source)` returns dist vector.

## Phase 8: Invariants & Verification (M)

- [ ] Add assert macros (guarded by ENABLE_BMSSP_VERIFIER) checking:
  - |P| ≤ |W|/k (when not early exit).
  - U_i disjoint across iterations.
  - Partial: k*2^l*t ≤ |U| ≤ 4k*2^l*t.
  - Success: B' == B & U == U~ (via reconstruction / BFS limited to B for debug only).
  - |W| ≤ k|S| unless early exit path chosen.
- [ ] Collect counters: relax_equal, relax_improve, inserts, batch_prepends, pulls.

## Phase 9: Testing Suite (H)

- [ ] Unit: structure Insert / BatchPrepend / Pull boundary correctness.
- [ ] Unit: FindPivots size bounds & pivot selection synthetic trees.
- [ ] BaseCase: boundary logic at k and k+1 threshold.
- [ ] BMSSP small graphs compare with Dijkstra (distances exact).
- [ ] Random constant-degree graphs (seeded) vs Dijkstra (fast path).
- [ ] Partial execution scenario tests (force by artificially low k or B).
- [ ] Edge case: multiple equal-distance paths (test tie-handling & uniqueness assumption surrogate order).

## Phase 10: Performance & Instrumentation (M)

- [ ] Benchmark harness (optional) measuring inserts per vertex, relax per edge.
- [ ] Dump stats (when enabled) summarizing amortized costs.
- [ ] Validate no edge inserted twice via Insert (track seen per level).

## Phase 11: Constant-Degree Transform (opt, L)

- [ ] Implement transformation to degree ≤ 2 (per paper) or assert input already limited.
- [ ] Map original vertex ids back to distances.
- [ ] Add CMake `ENABLE_CONST_DEGREE_TRANSFORM` (OFF default). When OFF and `ENABLE_BMSSP_VERIFIER` ON, verify max in/out degree ≤ 2 else warn or abort if `CONST_DEGREE_STRICT_MODE` ON.
- [ ] Provide statistics: max out-degree, max in-degree, number of transformed super-nodes.

## Phase 12: Documentation (M)

- [ ] Update README with BMSSP overview, parameters, limitations.
- [ ] Add doc comments referencing paper lines (Algorithm steps) per function.
- [ ] Provide invariants summary & debug flag usage.

## Phase 13: Cleanup & Migration (M)

- [ ] Remove legacy prototype from build (keep source for historical diff / move to docs/examples).
- [ ] Resolve compiler warnings (sign conversion, unused vars).
- [ ] Clang-Tidy configuration for future maintenance.

## Phase 14: Stretch Improvements (opt)

- [ ] Replace std::map in D1 with a B-tree / order-statistics tree if needed.
- [ ] Add parallel relaxation (thread pool) for large layers (guarded by flag).
- [ ] Memory pooling for blocks in 𝒟.

---

## Risk & Mitigation Notes

- Equality relax semantics: ensure edge reused across levels (store a flag when equality was processed).
- Subtree computation cost: restrict DFS to W; early abort after reaching k for pivot decision.
- BatchPrepend large L: median-of-medians adds complexity; initial implementation may accept O(L log L) with todo for optimization.
- Parameter scaling for small n: clamp k,t ≥ 1 to avoid division by zero or zero exponent issues.

---

## Acceptance Criteria Summary

- Correctness: Distances match Dijkstra for test suite; all assertions pass under verifier.
- Asymptotic behavior: Observed |P|/|S| shrink and |U| growth within theoretical bounds on random graphs.
- Code quality: Zero warnings in BMSSP sources under -Wall -Wextra -Wconversion (except documented intentional casts).
- Documentation: README + inline references to Algorithms 1–3 and Lemmas 3.2/3.3.

---

## Decision Log (resolved former open questions)

- Constant-degree handling: optional transform (`ENABLE_CONST_DEGREE_TRANSFORM`), otherwise verifier checks degree constraints.
- Distance representation: widening integer strategy with configurable minimum bit width and optional bigint fallback; avoid floating point.
- Pluggable weight type abstraction: deferred (future template refactor) to keep initial implementation focused & stable.

---

## Initial Ordering Recommendation

1. Phase 1–2 (scaffold + state)  
2. Phase 4 (𝒟)  
3. Phase 3 (FindPivots)  
4. Phase 5 (BaseCase)  
5. Phase 6 (BMSSP recursion)  
6. Phase 7 (driver)  
7. Phases 8–9 (verification + tests)  
8. Cleanup & docs (12–13)  

---
Generated roadmap stored in version control. Update this file as tasks complete.
