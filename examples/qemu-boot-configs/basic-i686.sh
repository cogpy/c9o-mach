#!/bin/bash
# Basic i686 QEMU Boot Configuration
# Inspired by: guile-daemon-zero's example configurations
#
# Quick boot test for the i686 kernel build.
# Usage: ./examples/qemu-boot-configs/basic-i686.sh [kernel-image]

set -euo pipefail

KERNEL="${1:-build-i686/gnumach}"
MEMORY="256M"
TIMEOUT=30

if [ ! -f "$KERNEL" ]; then
    echo "Kernel image not found: $KERNEL"
    echo "Build with: make -C build-i686"
    exit 1
fi

echo "Booting c9o-mach (i686) in QEMU..."
echo "  Kernel: $KERNEL"
echo "  Memory: $MEMORY"
echo "  Timeout: ${TIMEOUT}s"

qemu-system-i386 \
    -kernel "$KERNEL" \
    -m "$MEMORY" \
    -cpu pentium3-v1 \
    -no-reboot \
    -nographic \
    -serial stdio \
    -monitor none \
    -append "console=com0" \
    &

QEMU_PID=$!

# Wait for boot or timeout
sleep "$TIMEOUT"
kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true

echo "Boot test completed."
