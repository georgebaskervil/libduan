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
  - `BMSSP_FAIL_FAST_TESTS` (ON default) – when OFF, aggregate & report all mismatches.
  - `BMSSP_REQUIRE_EXACT_VERIFY` (ON default) – guard to prevent approximate modes.
  - `BMSSP_AGGREGATE_TEST_ERRORS` (OFF default) alias / secondary toggle (mutually exclusive with fail-fast when ON).
  - [x] Internal simple Dijkstra retained only for micro-bench / diagnostic (never authoritative for pass/fail when Boost present).
- [x] Decide numeric types & precision handling strategy: integer distances with widening + optional bigint fallback (see Phase 2 / 2b).

## Phase 1: Module Scaffolding (H)

- [x] Create directory `include/cpp_starter/bmssp/` & `src/bmssp/`.
- [x] Split interfaces:
  - graph.hpp
  - state.hpp (DistanceState / Relaxation API)
  - structure.hpp (Data structure 𝒟 interface per Lemma 3.3)
  - find_pivots.hpp / .cpp
  - base_case.hpp / .cpp
  - bmssp.hpp / .cpp (recursive engine + top level driver)
  - verify.hpp (instrumentation/asserts)
- [x] Update CMake to include new headers and legacy file behind option.
- [x] Add feature flag `ENABLE_BMSSP_VERIFIER` placeholder (flag present; logic partial).
- [x] Provide temporary minimal BMSSP (FindPivots/BaseCase/BMSSP) stub for tests (will be replaced by faithful versions in later phases).

## Phase 2: Distance / State Representation (H)

- [x] Implement `DistanceState` containing dist[], pred[], hop_count[], complete[].
- [x] `relax` with widening + lexicographic tie-break (dist, hop, vertex id) implemented.
- [x] Mark complete + initialize sources helpers.
- [x] Integer widening chain 32→64→128→BigInt (optional) implemented.
- [x] Overflow detection & widening for endpoints (global structure resize not yet required).
- [x] CMake options integrated (`MIN_DISTANCE_BITS`, `ENABLE_DISTANCE_WIDENING`, `ENABLE_BIGINT_FALLBACK`).
- [x] Verifier logging: counters for attempts, accept/reject, widen/overflow, widen_bytes and big_bytes.
- [~] Failure semantics: simulated OOM hook present; rollback not implemented.
- [x] Final early-accept refactor for INF + BIG source; tests passing.

## Phase 2b: Distance Scaling & Reliability (M)

- [x] Boundary stress tests (2^32, 2^64 edges).
- [x] Post-widen invariants (no change to completed vertices).
- [x] Benchmark widening overhead.
- [x] Fast-path skip overflow checks when safe.
- [x] README reliability trade-offs.

## Phase 3: Faithful FindPivots (Algorithm 1) (H)

- [x] Implement layered relaxation for exactly k iterations (or early termination) with bound B.
- [x] Maintain W_i layers; union into W.
- [x] Early exit condition: if |W| > k|S| then return P=S, W current.
- [x] Build forest F: edges (u,v) where u,v ∈ W and dist[v] == dist[u] + w_uv after final relaxation.
- [x] Compute subtree sizes in F for each root in S; mark pivots where subtree size ≥ k.
- [x] Return (P,W); ensure P ⊆ S, |P| ≤ |W|/k. (Trim safeguard if over)
- [~] Instrumentation asserts: |W| ≤ k|S| when no early exit. (|U~| bound requires later context)

## Phase 4: Data Structure 𝒟 (Lemma 3.3) (H)

- [~] Design block structures:  
  - Each block should contain:
    - A `vector` (or similar sequence) of elements.
    - An `upper_bound` value representing the maximum key in the block.
- [ ] Two sequences: D0 (batch prepends) + D1 (inserts) with std::map keyed by block upper bound for D1 ordering.
- [x] Operations:
  - Initialize(M, B)
  - Insert(key,value) (update if lower)
  - BatchPrepend(list\<key,value\>) (partition into blocks ≤ M using median selection / nth_element)
  - Pull() -> (S_i (≤ M keys), B_i boundary)
  - Delete internal (used in Pull)
- [ ] Support complexity heuristics; add counters for amortized analysis (instrumentation only).
- [ ] Provide fallback simple structure behind a compile flag for debugging.

Notes (current): Implemented a correct, simple 𝒟 using a min-heap plus best-value dedup; supports Initialize/Insert/BatchPrepend/Pull with up-to-M returns and boundary. Block-based D0/D1 layout and amortized-complexity instrumentation are deferred.

## Phase 5: BaseCase (Algorithm 2) (M)

- [x] Preconditions: |S| == 1, source x is complete. (asserts under ENABLE_BMSSP_VERIFIER)
- [x] Mini Dijkstra limited by bound B; heap keyed by dist.
- [x] Stop when heap empty OR collected k+1 vertices.
- [x] If collected ≤ k: B' = B; U = U0.
- [x] Else B' = max dist in U0; U = {v ∈ U0 | dist[v] < B'}.
- [x] Mark all U vertices complete.
- [x] Assert invariants. (enabled when ENABLE_BMSSP_VERIFIER)

Notes: Implemented in `src/base_case.cpp` using DistWord-aware comparisons; returns `{boundary, U}` and marks U complete. Preconditions and invariants are asserted when verifier is enabled.

## Phase 6: Recursive BMSSP (Algorithm 3) (H)

- [x] Implement procedure BMSSP(l,B,S):
  - [x] If l==0 -> BaseCase.
  - [x] Run FindPivots -> (P,W).
  - [x] Initialize 𝒟 with M = 2^{l-1} * t, B.
  - [x] Insert each x∈P with dist[x].
  - [x] Track lower boundary implicitly (no explicit B0'; first pull determines B_i).
  - [x] Loop iterations:
    1. [x] Pull -> (S_i, B_i)
    2. [x] Recursive call BMSSP(l-1, B_i, S_i) -> (B_i', U_i)
    3. [x] Add U_i into U (track disjointness) and mark complete.
    4. [x] Relax edges from U_i. For each relaxation:
       - [x] If dist in [B_i, B): Insert
       - [x] Else if in [B_i', B_i): add to K
    5. [x] BatchPrepend K plus qualifying S_i vertices (dist in [B_i', B_i)).
    6. [x] If 𝒟 empty -> success; B' = B; break.
    7. [x] If |U| ≥ k *2^l* t -> partial; B' = B_i'; break.
  - [x] After loop add W' = {x∈W | dist[x] < B'} to U.
  - [x] Assert: success vs partial conditions & size bounds (Lemma 3.9).
- [x] Return (B', U).

Notes (current): Implemented in `src/bmssp.cpp` with DistWord-aware comparisons and the simple 𝒟. Remaining: verifier asserts for success/partial bounds.

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

- [ ] Data structure 𝒟 unit tests (Insert / BatchPrepend / Pull).
- [ ] FindPivots size bounds & synthetic pivot selection.
- [ ] BaseCase k / k+1 threshold behavior.
- [x] BMSSP recursion vs Dijkstra (small graphs). (tests/recursion_against_dijkstra_test.cpp)
- [ ] Random constant-degree graphs vs Dijkstra (parameterized seeds/sizes).
- [ ] Partial execution scenario tests.
- [ ] Equal-distance multi-path tie-handling.
- [x] BigInt widening / promotion tests for relax and INF acceptance.

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

- [x] Legacy prototype removed.
- [ ] Resolve compiler warnings:
  - [~] sign-conversion (most fixed; remaining in tests & possibly some casts).
  - [x] unused parameter cleanup.
  - [ ] test EXPECT_GE index warning.
  - [~] BigInt path unit test exists but failing (logic under repair).
- [ ] Add `.clang-tidy` config.
- [ ] Create `legacy-bmssp-prototype` tag & CHANGELOG entry.
- [ ] CHANGELOG: document distance layer design & legacy removal.

### Phase 13 Notes (current status)

Legacy source removed; earlier stray build references traced to cache. Current blocker: BigInt relax acceptance semantics (INF destination) causing failing test. Debug instrumentation temporarily added in `relax` (remove after fix). Proceed next with unified early-INF acceptance + test split.

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

---

## Current Progress Snapshot (2025-08-26)

- Completed phases: 0, 1, 2. Phase 4 ops implemented (simple 𝒟). Phase 5 core logic implemented. Phase 6 recursion implemented with Lemma 3.9 asserts under verifier.
- Active debugging: none for Phase 2; failure rollback deferred to 2b.
- Not started: 7, 8, 10–12, 14.
- Partial: 2b (bench and README done), 3 (notes), 4 (block layout/instrumentation), 9, 13.
- Immediate priorities: add recursion tests vs Dijkstra (Phase 9), then top-level driver (Phase 7).
