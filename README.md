# duansalgorithm

A minimal C++20 starter using CMake, targeting macOS (clang++) with GoogleTest via FetchContent. Includes VS Code presets/tasks, clangd tooling, and CI (macOS + Ubuntu).

> License: Placeholder comment only. Add your own license text if needed.

## Requirements

- macOS with Xcode command line tools (clang, make)
- CMake >= 3.20
- Optional: Ninja (not required, we use Unix Makefiles by default)
- Optional: jemalloc (Homebrew: `brew install jemalloc`; Ubuntu: `sudo apt-get install -y libjemalloc-dev`)

## Quick start (macOS, zsh)

```zsh
# Configure (Debug)
cmake --preset debug

# Build
cmake --build --preset debug

# Run the app
./build/debug/bin/duansalgorithm

# Run tests
ctest --preset debug --output-on-failure
```

## Layout

- `src/` app and library sources
- `include/` public headers (exported)
- `tests/` GoogleTest unit tests (CTest integrated)
- `.vscode/` tasks and launch configs

## Notes

- C++ standard: C++20
- On CI we use warnings-as-errors.
- Default compilers on macOS are set to clang/clang++ in `CMakePresets.json`.
- jemalloc linking is optional (ENABLE_JEMALLOC=ON by default if present on system). To disable:

```zsh
cmake --preset debug -DENABLE_JEMALLOC=OFF
```

### Apply jemalloc globally

- Build-time global override (C++ new/delete):

```zsh
cmake --preset debug -DENABLE_GLOBAL_JEMALLOC=ON
cmake --build --preset debug
```

- Runtime preload (OS interposition):
  - macOS (Homebrew path):

  ```zsh
  DYLD_INSERT_LIBRARIES=/opt/homebrew/lib/libjemalloc.dylib ./build/debug/bin/duansalgorithm
  ```

  - Linux (typical path, adjust if needed):

  ```zsh
  LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2 ./build/debug/bin/duansalgorithm
  ```

## BMSSP distance reliability and performance

This project implements integer distance widening with optional BigInt fallback and a fast-path to skip overflow checks when provably safe. Use these build flags:

- ENABLE_DISTANCE_WIDENING (default ON): Automatically widen distances on overflow (32→64→128). If OFF, overflow throws.
- ENABLE_BIGINT_FALLBACK (default OFF): Promote beyond 128-bit to BigInt on overflow. When ON, unit tests add BigInt coverage.
- ENABLE_BMSSP_VERIFIER (default OFF): Enables counters and optional bench test output.

Fast-path configuration

You can configure a safe bound at runtime by setting `DistState::fastpath_u64_max_sum` (e.g., ≤ (n-1)·max_edge_weight). If `base + w` ≤ this bound, `relax` skips the overflow check on the hot path.

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
