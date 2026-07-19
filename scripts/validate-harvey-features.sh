#!/bin/bash
# Validate the Harvey OS feature ports: the 9P codec and the Plan 9
# utilities (UTF-8 codec, path canonicalizer, quicksort).
#
# Usage: ./scripts/validate-harvey-features.sh [build-dir]
#
# Steps:
#   1. Compile both test modules.
#   2. Boot each under QEMU and require the testlib success marker.
#   3. Confirm every ported header preserves the upstream MIT notice,
#      as that license requires.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log()   { echo -e "${GREEN}[INFO]${NC} $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

BUILD_DIR="${1:-build-i686}"
SUCCESS_MARKER="gnumach-test-success-and-reboot"

cd "$PROJECT_ROOT"

if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/Makefile" ]; then
    error "Build directory '$BUILD_DIR' is not configured."
    error "Configure first, for example:"
    error "  mkdir -p build-i686 && cd build-i686 && \\"
    error "  ../configure --host=i686-gnu CC='gcc -m32' LD='ld -melf_i386' \\"
    error "      USER_MIG='mig' USER_CPPFLAGS='-m32' USER_CFLAGS='-m32' MIG='mig'"
    exit 1
fi

run_one() {
    local test_name="$1"
    local module="tests/module-${test_name}"
    local run_log

    log "Compiling ${module}"
    if ! make -C "$BUILD_DIR" "${module}" > /dev/null 2>&1; then
        error "Compilation of ${module} failed."
        make -C "$BUILD_DIR" "${module}" 2>&1 | tail -30
        exit 1
    fi

    log "Booting run-${test_name} under QEMU"
    run_log="$(mktemp)"
    if ! timeout 300 make -C "$BUILD_DIR" "run-${test_name}" 2>&1 \
            | tee "$run_log" | grep -E "PASS|${SUCCESS_MARKER}|ninep|plan9"; then
        warn "No test output captured; inspecting log."
    fi
    if ! grep -q "$SUCCESS_MARKER" "$run_log"; then
        error "run-${test_name} did not produce '${SUCCESS_MARKER}'."
        tail -40 "$run_log"
        rm -f "$run_log"
        exit 1
    fi
    rm -f "$run_log"
    log "run-${test_name} passed."
}

log "Step 1/2: build and boot the ported tests"
run_one ninep
run_one plan9util

log "Step 2/2: license-notice check on ported headers"
NOTICE="Plan 9 Foundation"
PERMISSION="Permission is hereby granted"
for f in tests/include/ninep/ninep.h \
         tests/include/plan9/utf.h \
         tests/include/plan9/cleanname.h \
         tests/include/plan9/sort.h; do
    if ! grep -q "$NOTICE" "$f" || ! grep -q "$PERMISSION" "$f"; then
        error "Ported header $f is missing the upstream MIT notice."
        exit 1
    fi
done
log "All ported headers preserve the upstream MIT permission notice."

log "All Harvey feature-port validations passed."
