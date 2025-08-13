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
