#!/bin/bash
# build-iso.sh - Build bootable ISO images for c9o-mach
# Copyright (C) 2024 Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build-iso}"
OUTPUT_DIR="${OUTPUT_DIR:-$PROJECT_ROOT}"

# Default values
ISO_TYPE="${ISO_TYPE:-live}"
ARCH="${ARCH:-i686}"
GNUMACH="${GNUMACH:-gnumach}"
ROOTFS="${ROOTFS:-}"
INSTALLER="${INSTALLER:-}"
VERSION="${VERSION:-$(cat $PROJECT_ROOT/version.m4 2>/dev/null | grep -oP 'VERSION.*\[\K[^\]]+' || echo '0.0.0')}"

# Color output helpers
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Build bootable ISO images for c9o-mach.

Options:
    -t, --type TYPE      ISO type: live, install, full (default: live)
    -a, --arch ARCH      Architecture: i686, x86_64 (default: i686)
    -k, --kernel FILE    Path to gnumach kernel binary
    -r, --rootfs FILE    Path to rootfs image
    -i, --installer FILE Path to installer binary
    -o, --output DIR     Output directory (default: $PROJECT_ROOT)
    -b, --build-dir DIR  Build directory (default: $BUILD_DIR)
    -v, --version VER    Version string for ISO naming
    -c, --clean          Clean build directory before building
    -h, --help           Show this help message

Examples:
    $0 --type live --arch i686
    $0 --type install --kernel ../gnumach --rootfs rootfs.img
    $0 --type full --arch x86_64 --clean

EOF
    exit 0
}

check_dependencies() {
    log_info "Checking dependencies..."
    local missing=()
    
    for cmd in grub-mkrescue xorriso mformat mkisofs; do
        if ! command -v "$cmd" &>/dev/null; then
            missing+=("$cmd")
        fi
    done
    
    if [ ${#missing[@]} -gt 0 ]; then
        log_error "Missing dependencies: ${missing[*]}"
        log_info "Install with: sudo apt install -y grub-pc-bin xorriso mtools"
        exit 1
    fi
    
    log_success "All dependencies found"
}

prepare_iso_structure() {
    local iso_root="$1"
    
    log_info "Preparing ISO directory structure..."
    
    # Create directory structure
    mkdir -p "$iso_root/boot/grub"
    mkdir -p "$iso_root/boot/modules"
    mkdir -p "$iso_root/EFI/BOOT"
    
    # Copy kernel
    if [ -f "$GNUMACH" ]; then
        cp "$GNUMACH" "$iso_root/boot/gnumach"
        log_success "Kernel copied: $GNUMACH"
    else
        log_error "Kernel not found: $GNUMACH"
        exit 1
    fi
    
    # Copy rootfs if provided
    if [ -n "$ROOTFS" ] && [ -f "$ROOTFS" ]; then
        cp "$ROOTFS" "$iso_root/boot/modules/rootfs.img"
        log_success "Rootfs copied: $ROOTFS"
    elif [ -f "$PROJECT_ROOT/packaging/rootfs/minimal/rootfs.img" ]; then
        cp "$PROJECT_ROOT/packaging/rootfs/minimal/rootfs.img" "$iso_root/boot/modules/rootfs.img"
        log_success "Default minimal rootfs copied"
    else
        log_warning "No rootfs image provided - ISO may not boot properly"
    fi
    
    # Copy installer if building install ISO
    if [ "$ISO_TYPE" = "install" ] || [ "$ISO_TYPE" = "full" ]; then
        if [ -n "$INSTALLER" ] && [ -f "$INSTALLER" ]; then
            cp "$INSTALLER" "$iso_root/boot/modules/installer"
            log_success "Installer copied: $INSTALLER"
        elif [ -f "$PROJECT_ROOT/packaging/installer/installer" ]; then
            cp "$PROJECT_ROOT/packaging/installer/installer" "$iso_root/boot/modules/installer"
            log_success "Default installer copied"
        else
            log_warning "No installer binary found for install ISO"
        fi
    fi
}

generate_grub_config() {
    local iso_root="$1"
    local template=""
    
    log_info "Generating GRUB configuration for $ISO_TYPE ISO..."
    
    case "$ISO_TYPE" in
        live)
            template="$SCRIPT_DIR/grub.cfg.live.template"
            ;;
        install|full)
            template="$SCRIPT_DIR/grub.cfg.install.template"
            ;;
        *)
            log_error "Unknown ISO type: $ISO_TYPE"
            exit 1
            ;;
    esac
    
    if [ -f "$template" ]; then
        cp "$template" "$iso_root/boot/grub/grub.cfg"
        log_success "GRUB config generated from template"
    else
        # Generate minimal grub.cfg
        cat > "$iso_root/boot/grub/grub.cfg" <<EOF
set timeout=10
set default=0

menuentry "c9o-mach $ISO_TYPE" {
    multiboot /boot/gnumach console=com0 $ISO_TYPE root=ramdisk
    module /boot/modules/rootfs.img rootfs
    boot
}
EOF
        log_warning "Using generated minimal GRUB config (template not found)"
    fi
}

build_iso() {
    local iso_root="$BUILD_DIR/iso-$ISO_TYPE-$ARCH"
    local iso_name="c9o-mach-${ISO_TYPE}-${ARCH}-${VERSION}.iso"
    local iso_output="$OUTPUT_DIR/$iso_name"
    
    log_info "Building $ISO_TYPE ISO for $ARCH..."
    
    # Clean and prepare
    rm -rf "$iso_root"
    mkdir -p "$iso_root"
    
    # Prepare structure
    prepare_iso_structure "$iso_root"
    
    # Generate GRUB config
    generate_grub_config "$iso_root"
    
    # Build ISO with grub-mkrescue
    log_info "Creating ISO with grub-mkrescue..."
    
    grub-mkrescue \
        -o "$iso_output" \
        "$iso_root" \
        --product-name="c9o-mach" \
        --product-version="$VERSION" \
        2>&1 | while read -r line; do
            echo "  $line"
        done
    
    if [ -f "$iso_output" ]; then
        local iso_size=$(du -h "$iso_output" | cut -f1)
        log_success "ISO created: $iso_output ($iso_size)"
        
        # Generate checksum
        if command -v sha256sum &>/dev/null; then
            sha256sum "$iso_output" > "$iso_output.sha256"
            log_info "Checksum: $iso_output.sha256"
        fi
    else
        log_error "ISO creation failed"
        exit 1
    fi
    
    # Cleanup
    if [ "${KEEP_BUILD_DIR:-}" != "1" ]; then
        rm -rf "$iso_root"
    fi
    
    echo ""
    log_success "ISO build complete!"
    echo "  Output: $iso_output"
    echo "  Type: $ISO_TYPE"
    echo "  Architecture: $ARCH"
    echo "  Version: $VERSION"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            ISO_TYPE="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -k|--kernel)
            GNUMACH="$2"
            shift 2
            ;;
        -r|--rootfs)
            ROOTFS="$2"
            shift 2
            ;;
        -i|--installer)
            INSTALLER="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -c|--clean)
            rm -rf "$BUILD_DIR"
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Main execution
check_dependencies
mkdir -p "$BUILD_DIR"
mkdir -p "$OUTPUT_DIR"
build_iso
