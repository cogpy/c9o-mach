#!/bin/bash
# Debug i686 QEMU Boot with GDB
# Inspired by: git9's clean modular approach, firedrake's debug infrastructure
#
# Boots the kernel with GDB stub enabled for remote debugging.
# Connect GDB with: target remote :1234
#
# Usage: ./examples/qemu-boot-configs/debug-i686.sh [kernel-image]

set -euo pipefail

KERNEL="${1:-build-i686-debug/gnumach}"
MEMORY="256M"
GDB_PORT=1234

if [ ! -f "$KERNEL" ]; then
    echo "Debug kernel image not found: $KERNEL"
    echo "Build with: ./configure --host=i686-gnu --enable-debug && make"
    exit 1
fi

echo "Booting c9o-mach (i686-debug) with GDB stub..."
echo "  Kernel: $KERNEL"
echo "  Memory: $MEMORY"
echo "  GDB port: $GDB_PORT"
echo ""
echo "Connect with: gdb -ex 'target remote :$GDB_PORT' $KERNEL"

qemu-system-i386 \
    -kernel "$KERNEL" \
    -m "$MEMORY" \
    -cpu pentium3-v1 \
    -no-reboot \
    -nographic \
    -serial stdio \
    -monitor none \
    -s -S \
    -append "console=com0"
