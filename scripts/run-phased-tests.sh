#!/bin/bash
# Phased Test Execution Script
# Inspired by: cogpwsh's phased test execution, cogfernos's multi-phase testing
#
# Executes tests in phases, from fast/basic to slow/comprehensive.
# Allows CI to fail fast on basic issues while still running full validation.
#
# Usage: ./scripts/run-phased-tests.sh [--phase N] [--all] [--verbose]

set -euo pipefail

PHASE=""
RUN_ALL=false
VERBOSE=false
PASS=0
FAIL=0
SKIP=0
TOTAL_PHASES=5

usage() {
    echo "Usage: $0 [--phase N] [--all] [--verbose]"
    echo ""
    echo "Phases:"
    echo "  1: Syntax & Build Validation"
    echo "  2: Unit Tests"
    echo "  3: Integration Tests (QEMU)"
    echo "  4: Static Analysis"
    echo "  5: Performance & Stress Tests"
    echo ""
    echo "Options:"
    echo "  --phase N   Run only phase N (1-$TOTAL_PHASES)"
    echo "  --all       Run all phases (default: phases 1-3)"
    echo "  --verbose   Show detailed output"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --phase) PHASE="$2"; shift 2 ;;
        --all) RUN_ALL=true; shift ;;
        --verbose) VERBOSE=true; shift ;;
        --help|-h) usage ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
done

log() { echo "[$(date '+%H:%M:%S')] $*"; }
pass() { ((PASS++)); log "  PASS: $1"; }
fail() { ((FAIL++)); log "  FAIL: $1"; }
skip() { ((SKIP++)); log "  SKIP: $1"; }

# ── Phase 1: Syntax & Build Validation ──
phase_1() {
    log "=== Phase 1: Syntax & Build Validation ==="

    # Check that key source files parse
    log "  Checking C source syntax..."
    local syntax_errors=0
    for dir in kern vm ipc device; do
        if [ -d "$dir" ]; then
            for f in "$dir"/*.c; do
                [ -f "$f" ] || continue
                if gcc -fsyntax-only -m32 -I include/ -I i386/include/ -I i386/ \
                    "$f" 2>/dev/null; then
                    $VERBOSE && log "    OK: $f"
                else
                    ((syntax_errors++))
                    $VERBOSE && log "    ERR: $f"
                fi
            done
        fi
    done

    if [ "$syntax_errors" -eq 0 ]; then
        pass "C source syntax check"
    else
        fail "C source syntax check ($syntax_errors errors)"
    fi

    # Check autotools files
    if [ -f configure.ac ]; then
        pass "configure.ac exists"
    else
        fail "configure.ac missing"
    fi

    if [ -f Makefile.am ]; then
        pass "Makefile.am exists"
    else
        fail "Makefile.am missing"
    fi

    # Check documentation completeness
    for doc in CONTRIBUTING.md SECURITY.md GOVERNANCE.md CHANGELOG.md; do
        if [ -f "$doc" ]; then
            pass "$doc exists"
        else
            skip "$doc missing"
        fi
    done
}

# ── Phase 2: Unit Tests ──
phase_2() {
    log "=== Phase 2: Unit Tests ==="

    if [ -d tests/ ]; then
        local test_count=0
        for test in tests/*.c tests/*.sh; do
            [ -f "$test" ] || continue
            ((test_count++))
            if [[ "$test" == *.sh ]]; then
                if bash -n "$test" 2>/dev/null; then
                    pass "$(basename "$test") (syntax)"
                else
                    fail "$(basename "$test") (syntax)"
                fi
            fi
        done
        if [ "$test_count" -eq 0 ]; then
            skip "No unit tests found"
        fi
    else
        skip "tests/ directory not found"
    fi

    # Run make check if available
    if make -n check >/dev/null 2>&1; then
        log "  Running make check..."
        if make check 2>&1 | tail -5; then
            pass "make check"
        else
            fail "make check"
        fi
    else
        skip "make check not available"
    fi
}

# ── Phase 3: Integration Tests (QEMU) ──
phase_3() {
    log "=== Phase 3: Integration Tests (QEMU) ==="

    if command -v qemu-system-i386 >/dev/null 2>&1; then
        if [ -x scripts/run-enhanced-tests.sh ]; then
            log "  Running QEMU integration tests..."
            if timeout 300 bash scripts/run-enhanced-tests.sh 2>&1 | tail -10; then
                pass "QEMU integration tests"
            else
                fail "QEMU integration tests"
            fi
        else
            skip "QEMU test script not found"
        fi
    else
        skip "QEMU not available"
    fi
}

# ── Phase 4: Static Analysis ──
phase_4() {
    log "=== Phase 4: Static Analysis ==="

    if command -v cppcheck >/dev/null 2>&1; then
        log "  Running cppcheck..."
        local cppcheck_errors
        cppcheck_errors=$(cppcheck --enable=warning,performance \
            --suppress=missingInclude --quiet \
            -I include/ kern/ vm/ ipc/ 2>&1 | grep -c "error" || true)
        if [ "$cppcheck_errors" -eq 0 ]; then
            pass "cppcheck (0 errors)"
        else
            fail "cppcheck ($cppcheck_errors errors)"
        fi
    else
        skip "cppcheck not available"
    fi

    if [ -x scripts/run-static-analysis.sh ]; then
        log "  Running extended static analysis..."
        if bash scripts/run-static-analysis.sh 2>&1 | tail -5; then
            pass "Extended static analysis"
        else
            fail "Extended static analysis"
        fi
    else
        skip "Static analysis script not found"
    fi

    # Check for unsafe functions
    local unsafe_count
    unsafe_count=$(grep -rn -E '\b(gets|sprintf|strcpy|strcat)\b' \
        --include="*.c" kern/ vm/ ipc/ 2>/dev/null | wc -l || true)
    if [ "$unsafe_count" -eq 0 ]; then
        pass "No unsafe function calls"
    else
        fail "Found $unsafe_count unsafe function calls"
    fi
}

# ── Phase 5: Performance & Stress Tests ──
phase_5() {
    log "=== Phase 5: Performance & Stress Tests ==="

    if [ -x scripts/benchmark-ipc.sh ]; then
        log "  Running IPC benchmarks..."
        if timeout 120 bash scripts/benchmark-ipc.sh 2>&1 | tail -5; then
            pass "IPC benchmark"
        else
            fail "IPC benchmark"
        fi
    else
        skip "IPC benchmark script not found"
    fi

    if [ -x scripts/stress-test-memory.sh ]; then
        log "  Running memory stress tests..."
        if timeout 120 bash scripts/stress-test-memory.sh 2>&1 | tail -5; then
            pass "Memory stress test"
        else
            fail "Memory stress test"
        fi
    else
        skip "Memory stress test script not found"
    fi

    # Stack analysis
    if [ -x scripts/checkstack.sh ]; then
        log "  Running stack analysis..."
        if bash scripts/checkstack.sh . 1024 2>&1 | tail -5; then
            pass "Stack analysis"
        else
            fail "Stack analysis (functions exceed threshold)"
        fi
    else
        skip "Stack analysis script not found"
    fi
}

# ── Main Execution ──
log "c9o-mach Phased Test Runner"
log "$(printf '%.0s=' {1..50})"

if [ -n "$PHASE" ]; then
    # Run single phase
    "phase_$PHASE"
elif $RUN_ALL; then
    # Run all phases
    for p in $(seq 1 $TOTAL_PHASES); do
        "phase_$p"
        echo ""
    done
else
    # Default: phases 1-3 (fast feedback)
    for p in 1 2 3; do
        "phase_$p"
        echo ""
    done
fi

log "$(printf '%.0s=' {1..50})"
log "Test Summary"
log "  PASS: $PASS"
log "  FAIL: $FAIL"
log "  SKIP: $SKIP"
log "  Total: $((PASS + FAIL + SKIP))"
log "$(printf '%.0s=' {1..50})"

[ "$FAIL" -eq 0 ] || exit 1
