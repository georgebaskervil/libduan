#!/usr/bin/env bash
set -euo pipefail

# Optional environment overrides:
#   ENABLE_BIGINT=ON/OFF
#   ENABLE_VERIFIER=ON/OFF
ENABLE_BIGINT=${ENABLE_BIGINT:-ON}
ENABLE_VERIFIER=${ENABLE_VERIFIER:-OFF}

echo "== Clean configure (ENABLE_BIGINT=${ENABLE_BIGINT} ENABLE_VERIFIER=${ENABLE_VERIFIER}) =="
REPO_ROOT=$(pwd)
rm -rf "${REPO_ROOT}/build/debug"

cmake --preset debug -DENABLE_BIGINT_FALLBACK=${ENABLE_BIGINT} -DENABLE_BMSSP_VERIFIER=${ENABLE_VERIFIER}

echo "== Build =="
cmake --build --preset debug

echo "== Basic test run =="
ctest --preset debug --output-on-failure || true

echo "== compile_commands sanity =="
test -f build/debug/compile_commands.json || { echo 'compile_commands.json missing'; exit 1; }
grep -F "legacy_prototype_bmssp.cpp" build/debug/compile_commands.json || echo 'Not in compile_commands'

echo "== Verbose build scan for legacy file =="
cmake --build --preset debug --verbose 2>&1 | grep -F "legacy_prototype_bmssp.cpp" || echo 'No legacy compile invocations in verbose build'

echo "== DependInfo scan =="
grep -R "legacy_prototype_bmssp.cpp" build/debug/CMakeFiles/*/DependInfo.cmake || echo 'No legacy in DependInfo'

echo "== Locate any copies of legacy_prototype_bmssp.cpp (depth 5) =="
find . -maxdepth 5 -type f -name legacy_prototype_bmssp.cpp -exec sh -c 'echo "-- {}"; md5 {}' \; || true

echo "== Other build trees containing compile_commands.json =="
find . -type f -name compile_commands.json -not -path './build/debug/*'

echo "== Verifier symbol probe =="
if nm build/debug/lib/libcpp_starter.a 2>/dev/null | grep -q relax_improve_events; then
	echo 'Verifier counters present.'
else
	echo 'Verifier counters NOT present.'
fi

echo "== Summary =="
echo "BigInt: ${ENABLE_BIGINT} | Verifier: ${ENABLE_VERIFIER}"
echo 'Done.'