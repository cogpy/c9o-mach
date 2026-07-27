#!/bin/bash
# validate-iso.sh - Validate ISO image integrity and boot capability
# Copyright (C) 2024 Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[FAIL]${NC} $1"; }

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Test result tracking
test_pass() {
    TESTS_PASSED=$((TESTS_PASSED + 1))
    log_success "$1"
}

test_fail() {
    TESTS_FAILED=$((TESTS_FAILED + 1))
    log_error "$1"
}

test_skip() {
    TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
    log_warning "SKIP: $1"
}

usage() {
    cat <<EOF
Usage: $0 [OPTIONS] <iso-file>

Validate a c9o-mach ISO image.

Options:
    -b, --boot-test     Run QEMU boot test
    -e, --efi           Test EFI boot (requires OVMF)
    -t, --timeout SEC   Boot test timeout (default: 60)
    -v, --verbose       Verbose output
    -h, --help          Show this help

Examples:
    $0 c9o-mach-live-i686.iso
    $0 --boot-test c9o-mach-install-i686.iso
    $0 --efi --boot-test c9o-mach-live-x86_64.iso

EOF
    exit 0
}

# Check if ISO file exists and is valid
check_iso_file() {
    local iso="$1"
    
    log_info "Checking ISO file: $iso"
    
    # Check file exists
    if [ ! -f "$iso" ]; then
        test_fail "ISO file does not exist: $iso"
        return 1
    fi
    test_pass "ISO file exists"
    
    # Check file is not empty
    local size=$(stat -c%s "$iso" 2>/dev/null || stat -f%z "$iso" 2>/dev/null)
    if [ "$size" -lt 1024 ]; then
        test_fail "ISO file too small: $size bytes"
        return 1
    fi
    test_pass "ISO file size: $size bytes"
    
    # Check ISO magic number
    local magic=$(xxd -l 5 -s 32769 "$iso" 2>/dev/null | awk '{print $2$3$4$5$6}')
    if [[ "$magic" == "4344303031" ]]; then
        test_pass "Valid ISO 9660 signature found"
    else
        test_fail "Invalid ISO 9660 signature"
        return 1
    fi
    
    return 0
}

# Check ISO contents
check_iso_contents() {
    local iso="$1"
    
    log_info "Checking ISO contents..."
    
    # Check for required files using isoinfo if available
    if command -v isoinfo &>/dev/null; then
        local contents=$(isoinfo -l -i "$iso" 2>/dev/null || true)
        
        # Check for kernel
        if echo "$contents" | grep -qi "gnumach"; then
            test_pass "Kernel (gnumach) found in ISO"
        else
            test_fail "Kernel (gnumach) not found in ISO"
        fi
        
        # Check for GRUB
        if echo "$contents" | grep -qi "grub"; then
            test_pass "GRUB bootloader found in ISO"
        else
            test_fail "GRUB bootloader not found in ISO"
        fi
        
        # Check for grub.cfg
        if echo "$contents" | grep -qi "grub.cfg"; then
            test_pass "GRUB configuration found in ISO"
        else
            test_warning "GRUB configuration not found (may be embedded)"
        fi
    else
        test_skip "isoinfo not available - cannot check contents"
    fi
}

# Check ISO checksum if available
check_checksum() {
    local iso="$1"
    local checksum_file="${iso}.sha256"
    
    log_info "Checking checksum..."
    
    if [ -f "$checksum_file" ]; then
        if sha256sum -c "$checksum_file" &>/dev/null; then
            test_pass "SHA256 checksum verified"
        else
            test_fail "SHA256 checksum mismatch"
            return 1
        fi
    else
        test_skip "No checksum file found: $checksum_file"
    fi
    
    return 0
}

# Run QEMU boot test
run_boot_test() {
    local iso="$1"
    local timeout="${2:-60}"
    local efi_mode="${3:-0}"
    local log_file="/tmp/iso-boot-test-$$.log"
    
    log_info "Running boot test (timeout: ${timeout}s)..."
    
    # Determine QEMU binary and options
    local qemu_bin="qemu-system-i386"
    local qemu_opts="-m 512 -nographic -no-reboot -boot d"
    
    # Check if x86_64 ISO
    if echo "$iso" | grep -q "x86_64"; then
        qemu_bin="qemu-system-x86_64"
    fi
    
    # Add EFI support if requested
    if [ "$efi_mode" = "1" ]; then
        local ovmf_path="/usr/share/OVMF/OVMF_CODE.fd"
        if [ ! -f "$ovmf_path" ]; then
            ovmf_path="/usr/share/edk2/ovmf/OVMF_CODE.fd"
        fi
        
        if [ -f "$ovmf_path" ]; then
            qemu_opts+=" -bios $ovmf_path"
            log_info "Using UEFI firmware: $ovmf_path"
        else
            test_skip "OVMF not found - cannot test EFI boot"
            return 0
        fi
    fi
    
    # Check QEMU availability
    if ! command -v "$qemu_bin" &>/dev/null; then
        test_skip "QEMU not available: $qemu_bin"
        return 0
    fi
    
    # Run QEMU with timeout
    log_info "Starting QEMU: $qemu_bin"
    
    timeout --foreground "$timeout" \
        $qemu_bin $qemu_opts -cdrom "$iso" 2>&1 | tee "$log_file" &
    local qemu_pid=$!
    
    # Wait and check output
    sleep 5  # Give it time to start
    
    # Look for boot indicators
    local boot_success=0
    
    # Check log for positive indicators
    if grep -qi "GNU Mach" "$log_file" 2>/dev/null; then
        boot_success=1
    elif grep -qi "c9o-mach" "$log_file" 2>/dev/null; then
        boot_success=1
    elif grep -qi "Loading kernel" "$log_file" 2>/dev/null; then
        boot_success=1
    fi
    
    # Wait for QEMU to finish or timeout
    wait $qemu_pid 2>/dev/null || true
    
    # Check for negative indicators
    if grep -qi "error" "$log_file" 2>/dev/null; then
        if grep -qi "fatal" "$log_file" 2>/dev/null; then
            test_fail "Boot failed with fatal error"
            [ -n "${VERBOSE:-}" ] && cat "$log_file"
            rm -f "$log_file"
            return 1
        fi
    fi
    
    # Evaluate result
    if [ "$boot_success" = "1" ]; then
        test_pass "Boot test succeeded - kernel loaded"
    else
        test_fail "Boot test failed - no kernel load detected"
        [ -n "${VERBOSE:-}" ] && cat "$log_file"
    fi
    
    rm -f "$log_file"
    return 0
}

# Main validation function
validate_iso() {
    local iso="$1"
    local do_boot_test="${2:-0}"
    local efi_mode="${3:-0}"
    local timeout="${4:-60}"
    
    echo "========================================"
    echo "c9o-mach ISO Validation"
    echo "========================================"
    echo "ISO: $iso"
    echo "Boot test: $([ "$do_boot_test" = "1" ] && echo "yes" || echo "no")"
    echo "EFI mode: $([ "$efi_mode" = "1" ] && echo "yes" || echo "no")"
    echo "========================================"
    echo ""
    
    # Run validation checks
    check_iso_file "$iso" || exit 1
    check_iso_contents "$iso"
    check_checksum "$iso"
    
    # Run boot test if requested
    if [ "$do_boot_test" = "1" ]; then
        run_boot_test "$iso" "$timeout" "$efi_mode"
    fi
    
    # Summary
    echo ""
    echo "========================================"
    echo "Validation Summary"
    echo "========================================"
    echo -e "Passed:  ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed:  ${RED}$TESTS_FAILED${NC}"
    echo -e "Skipped: ${YELLOW}$TESTS_SKIPPED${NC}"
    echo "========================================"
    
    if [ "$TESTS_FAILED" -gt 0 ]; then
        echo -e "${RED}VALIDATION FAILED${NC}"
        exit 1
    else
        echo -e "${GREEN}VALIDATION PASSED${NC}"
        exit 0
    fi
}

# Parse arguments
DO_BOOT_TEST=0
EFI_MODE=0
TIMEOUT=60
VERBOSE=""
ISO_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--boot-test)
            DO_BOOT_TEST=1
            shift
            ;;
        -e|--efi)
            EFI_MODE=1
            shift
            ;;
        -t|--timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            ISO_FILE="$1"
            shift
            ;;
    esac
done

# Check for ISO file argument
if [ -z "$ISO_FILE" ]; then
    echo "Error: No ISO file specified"
    echo ""
    usage
fi

# Run validation
validate_iso "$ISO_FILE" "$DO_BOOT_TEST" "$EFI_MODE" "$TIMEOUT"
