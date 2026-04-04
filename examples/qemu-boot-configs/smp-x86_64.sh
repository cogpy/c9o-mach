#!/bin/bash
# SMP x86_64 QEMU Boot Configuration
# Inspired by: lustre's multi-node testing, mardukros's performance monitoring
#
# Boots x86_64 kernel with multiple CPUs for SMP testing.
# Usage: ./examples/qemu-boot-configs/smp-x86_64.sh [kernel-image] [num-cpus]

set -euo pipefail

KERNEL="${1:-build-x86_64/gnumach}"
NUM_CPUS="${2:-4}"
MEMORY="512M"
TIMEOUT=60

if [ ! -f "$KERNEL" ]; then
    echo "Kernel image not found: $KERNEL"
    echo "Build with: ./configure --host=x86_64-gnu && make"
    exit 1
fi

echo "Booting c9o-mach (x86_64 SMP) in QEMU..."
echo "  Kernel: $KERNEL"
echo "  CPUs: $NUM_CPUS"
echo "  Memory: $MEMORY"
echo "  Timeout: ${TIMEOUT}s"

qemu-system-x86_64 \
    -kernel "$KERNEL" \
    -m "$MEMORY" \
    -smp "$NUM_CPUS" \
    -cpu core2duo-v1 \
    -no-reboot \
    -nographic \
    -serial stdio \
    -monitor none \
    -append "console=com0" \
    &

QEMU_PID=$!

sleep "$TIMEOUT"
kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true

echo "SMP boot test completed."
