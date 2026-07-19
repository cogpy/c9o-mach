#!/bin/bash
# Valgrind Memory Testing Script
# Inspired by: diod's comprehensive Valgrind integration in CI
#
# Runs user-space test binaries under Valgrind to detect:
# - Memory leaks
# - Use-after-free
# - Buffer overflows
# - Uninitialized memory reads
#
# Usage: ./scripts/run-valgrind-tests.sh [test-binary ...]

set -euo pipefail

VALGRIND_OPTS=(
    --tool=memcheck
    --leak-check=full
    --show-leak-kinds=all
    --track-origins=yes
    --error-exitcode=1
    --errors-for-leak-kinds=definite,possible
    --suppressions=scripts/valgrind.supp
    --xml=yes
)

RESULTS_DIR="test-results/valgrind"
mkdir -p "$RESULTS_DIR"

PASS=0
FAIL=0
SKIP=0

run_valgrind() {
    local binary="$1"
    local name
    name=$(basename "$binary")
    local xml_file="$RESULTS_DIR/${name}.xml"

    echo "=== Valgrind: $name ==="

    if [ ! -x "$binary" ]; then
        echo "  SKIP (not executable)"
        (( ++SKIP ))
        return 0
    fi

    local -a opts=("${VALGRIND_OPTS[@]}" "--xml-file=$xml_file")

    if valgrind "${opts[@]}" "$binary" 2>&1; then
        echo "  PASS (no memory errors)"
        (( ++PASS ))
    else
        echo "  FAIL (memory errors detected)"
        (( ++FAIL ))
    fi
}

# Find test binaries
if [ $# -gt 0 ]; then
    BINARIES=("$@")
else
    BINARIES=()
    for dir in tests/ test-results/; do
        if [ -d "$dir" ]; then
            while IFS= read -r -d '' bin; do
                BINARIES+=("$bin")
            done < <(find "$dir" -type f -executable -print0 2>/dev/null)
        fi
    done
fi

if [ ${#BINARIES[@]} -eq 0 ]; then
    echo "No test binaries found. Build with 'make check' first."
    exit 0
fi

echo "Running Valgrind on ${#BINARIES[@]} test binaries..."
echo "Results directory: $RESULTS_DIR"
echo ""

for binary in "${BINARIES[@]}"; do
    run_valgrind "$binary"
    echo ""
done

echo "==============================="
echo "Valgrind Summary"
echo "  PASS: $PASS"
echo "  FAIL: $FAIL"
echo "  SKIP: $SKIP"
echo "  Total: $((PASS + FAIL + SKIP))"
echo "==============================="

[ "$FAIL" -eq 0 ] || exit 1
