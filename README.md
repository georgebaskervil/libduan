# duansalgorithm

A minimal C++20 starter using CMake, targeting macOS (clang++) with GoogleTest via FetchContent. Includes VS Code presets/tasks, clangd tooling, and CI (macOS + Ubuntu).

> License: Placeholder comment only. Add your own license text if needed.

## Requirements
- macOS with Xcode command line tools (clang, make)
- CMake >= 3.20
- Optional: Ninja (not required, we use Unix Makefiles by default)

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
