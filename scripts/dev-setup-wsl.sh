#!/usr/bin/env bash
set -euo pipefail

echo "Installing GNU Mach development dependencies..."

# Core build dependencies
sudo apt update
sudo apt install -y build-essential gcc-multilib binutils binutils-multiarch \
  autoconf automake libtool pkg-config gawk bison flex nasm \
  xorriso grub-pc-bin mtools qemu-system-x86 \
  git python3 cppcheck clang-tools texinfo || true

# Packaging dependencies for ISO building
echo "Installing packaging dependencies..."
sudo apt install -y \
  grub-common grub-pc-bin grub-efi-ia32-bin grub-efi-amd64-bin \
  e2fsprogs dosfstools parted gdisk \
  ovmf \
  squashfs-tools cpio genisoimage || true

# MIG (Mach Interface Generator) installation
if ! command -v mig >/dev/null 2>&1; then
  sudo apt install -y mig || true
fi

if ! command -v mig >/dev/null 2>&1; then
  # Use the local mig directory instead of downloading
  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  MIG_DIR="$SCRIPT_DIR/../mig"
  
  if [ ! -d "$MIG_DIR" ]; then
    echo "Error: Local mig directory not found at $MIG_DIR"
    exit 1
  fi
  
  pushd "$MIG_DIR"
  # Check if bootstrap has already been run
  if [ ! -f "configure" ]; then
    ./bootstrap
  fi
  # Check if already configured
  if [ ! -f "Makefile" ]; then
    ./configure
  fi
  make -j"$(nproc)"
  sudo make install
  popd
fi

echo ""
echo "============================================="
echo "WSL development dependencies installed."
echo ""
echo "Build commands:"
echo "  autoreconf --install && ./configure --host=i686-gnu CC='gcc -m32' LD='ld -melf_i386'"
echo "  make -j\$(nproc)"
echo ""
echo "Testing commands:"
echo "  make check                    # Run all tests"
echo "  make run-hello                # Run basic test"
echo ""
echo "ISO building (configure with --enable-live-iso --enable-installer):"
echo "  make iso-live                 # Build live boot ISO"
echo "  make iso-install              # Build installation ISO"
echo "  make iso-test                 # Test ISO in QEMU"
echo "============================================="

