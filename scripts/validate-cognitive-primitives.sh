#!/bin/bash
# Validate the cognitive primitives library and its QEMU boot test.
#
# Usage: ./scripts/validate-cognitive-primitives.sh [build-dir]
#
# Steps:
#   1. Compile the test module (fast failure on header or test errors).
#   2. Boot it under QEMU and require the testlib success marker.
#   3. Static consistency checks: reserved MIG subsystem number is not
#      yet taken, documentation cross-references resolve, and the
#      cognitive sources contain only ASCII.

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

log "Step 1/3: compiling tests/module-cognitive in $BUILD_DIR"
if ! make -C "$BUILD_DIR" tests/module-cognitive > /dev/null 2>&1; then
    error "Compilation of the cognitive test module failed."
    make -C "$BUILD_DIR" tests/module-cognitive 2>&1 | tail -30
    exit 1
fi
log "Compilation succeeded."

log "Step 2/3: booting the cognitive test under QEMU"
RUN_LOG="$(mktemp)"
trap 'rm -f "$RUN_LOG"' EXIT
if ! timeout 300 make -C "$BUILD_DIR" run-cognitive 2>&1 | tee "$RUN_LOG" \
        | grep -E "cognitive|PASS|$SUCCESS_MARKER" ; then
    warn "No cognitive output captured; inspecting log."
fi
if ! grep -q "$SUCCESS_MARKER" "$RUN_LOG"; then
    error "QEMU run did not produce the success marker '$SUCCESS_MARKER'."
    tail -40 "$RUN_LOG"
    exit 1
fi
log "QEMU boot test passed."

log "Step 3/3: static consistency checks"

# The cognitive MIG subsystem number 5400 is documented as reserved for
# the Phase 3 graduation; it must not be claimed by other interfaces.
if grep -rn "subsystem[a-zA-Z_ ]*5400" include/mach/*.defs include/device/*.defs 2>/dev/null; then
    error "MIG subsystem 5400 is already in use; docs/cogwxp-integration.md must be updated."
    exit 1
fi
log "Reserved subsystem number 5400 is still free."

DOC="docs/cogwxp-integration.md"
if [ -f "$DOC" ]; then
    missing=0
    while IFS= read -r path; do
        if [ ! -e "$path" ]; then
            error "Path referenced by $DOC does not exist: $path"
            missing=1
        fi
    done < <(grep -oE '`(tests|scripts|include|kern|docs)/[A-Za-z0-9_./-]+`' "$DOC" \
             | tr -d '\`' | sort -u)
    if [ "$missing" -ne 0 ]; then
        exit 1
    fi
    log "All repository paths referenced by $DOC exist."
else
    warn "$DOC not present; skipping cross-reference check."
fi

for f in tests/include/cognitive/cogpln.h \
         tests/include/cognitive/cogspace.h \
         tests/test-cognitive.c; do
    if LC_ALL=C grep -qP '[^\x00-\x7F]' "$f"; then
        error "Non-ASCII content found in $f"
        exit 1
    fi
done
log "Cognitive sources are ASCII-clean."

log "All cognitive primitives validations passed."
