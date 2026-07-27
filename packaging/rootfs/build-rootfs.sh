#!/usr/bin/env bash
# Build rootfs image for c9o-mach bootable ISOs
#
# This script creates a minimal root filesystem image that can be loaded
# as a multiboot module by GRUB.

set -euo pipefail

# Script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default configuration
ROOTFS_SIZE="${ROOTFS_SIZE:-32}"  # Size in MB
ROOTFS_TYPE="${ROOTFS_TYPE:-minimal}"
OUTPUT_FILE="${OUTPUT_FILE:-rootfs.img}"
BUILD_DIR="${BUILD_DIR:-$TOP_DIR/rootfs-build}"
ARCH="${ARCH:-i686}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Build a root filesystem image for c9o-mach

Options:
    -s, --size SIZE     Image size in MB (default: $ROOTFS_SIZE)
    -t, --type TYPE     Rootfs type: minimal or full (default: $ROOTFS_TYPE)
    -o, --output FILE   Output filename (default: $OUTPUT_FILE)
    -a, --arch ARCH     Architecture: i686 or x86_64 (default: $ARCH)
    -b, --build-dir DIR Build directory (default: $BUILD_DIR)
    -h, --help          Show this help message
    -v, --verbose       Enable verbose output
    
Examples:
    $0                          # Build minimal rootfs
    $0 -s 64 -t full            # Build full rootfs (64MB)
    $0 -o my-rootfs.img         # Custom output filename
EOF
}

VERBOSE=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--size)
            ROOTFS_SIZE="$2"
            shift 2
            ;;
        -t|--type)
            ROOTFS_TYPE="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Check dependencies
check_deps() {
    local missing=()
    
    for cmd in dd; do
        if ! command -v "$cmd" &>/dev/null; then
            missing+=("$cmd")
        fi
    done
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing[*]}"
        exit 1
    fi
}

# Create minimal rootfs structure
create_minimal_rootfs() {
    log_info "Creating minimal rootfs structure..."
    
    local rootfs_dir="$BUILD_DIR/rootfs"
    mkdir -p "$rootfs_dir"/{boot,dev,etc,lib,mnt,proc,root,sbin,srv,sys,tmp,var}
    
    # Create essential device nodes (symbolic for now - real ones created at boot)
    mkdir -p "$rootfs_dir/dev"
    
    # Create basic etc files
    cat > "$rootfs_dir/etc/hostname" <<EOF
c9o-mach
EOF

    cat > "$rootfs_dir/etc/fstab" <<EOF
# c9o-mach filesystem table
# <device>      <mountpoint>    <type>    <options>    <dump>  <pass>
# Root filesystem is configured at boot time
EOF

    cat > "$rootfs_dir/etc/motd" <<EOF

   ____  ___                              _     
  / ___|/ _ \ ___ ___ ___ _ __ ___   __ _| |___ 
 | |   | (_) | __/ _ \___| '_ \` _ \ / _\` | / __|
 | |___| (_) |  __/ (_) | | | | | | | (_| | \__ \\
  \____|  \___/\___\___/|_| |_| |_|\__,_|_|___/

  GNU Mach Microkernel
  
  Welcome to c9o-mach!
  
EOF

    # Create VERSION file
    if [[ -f "$TOP_DIR/version.m4" ]]; then
        VERSION=$(grep AC_PACKAGE_VERSION "$TOP_DIR/version.m4" | sed "s/.*\[\([^]]*\)\].*/\1/")
        echo "$VERSION" > "$rootfs_dir/etc/version"
    else
        echo "development" > "$rootfs_dir/etc/version"
    fi
    
    # Create basic init script placeholder
    cat > "$rootfs_dir/etc/init.rc" <<EOF
# c9o-mach init configuration
# Services to start at boot
console
disk
EOF

    log_info "Minimal rootfs structure created"
}

# Create full rootfs with additional components
create_full_rootfs() {
    # Start with minimal
    create_minimal_rootfs
    
    log_info "Adding full rootfs components..."
    
    local rootfs_dir="$BUILD_DIR/rootfs"
    
    # Additional directories for full install
    mkdir -p "$rootfs_dir"/{bin,home,opt,usr/{bin,lib,share,include}}
    
    # Add more configuration files
    cat > "$rootfs_dir/etc/shells" <<EOF
/bin/sh
EOF

    cat > "$rootfs_dir/etc/profile" <<EOF
# System-wide profile
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
export HOME=/root
export PS1='[\u@\h \W]\$ '
EOF

    log_info "Full rootfs structure created"
}

# Create the rootfs image file
create_image() {
    log_info "Creating rootfs image: $OUTPUT_FILE (${ROOTFS_SIZE}MB)"
    
    # Create empty image
    dd if=/dev/zero of="$BUILD_DIR/$OUTPUT_FILE" bs=1M count="$ROOTFS_SIZE" status=none
    
    log_info "Rootfs image created: $BUILD_DIR/$OUTPUT_FILE"
    
    # Pack the rootfs directory into a cpio archive within the image
    log_info "Packing rootfs contents..."
    
    local rootfs_dir="$BUILD_DIR/rootfs"
    
    if command -v cpio &>/dev/null; then
        # Create cpio archive
        (cd "$rootfs_dir" && find . | cpio -o -H newc 2>/dev/null) > "$BUILD_DIR/rootfs.cpio"
        
        # If gzip is available, compress it
        if command -v gzip &>/dev/null; then
            gzip -f "$BUILD_DIR/rootfs.cpio"
            mv "$BUILD_DIR/rootfs.cpio.gz" "$BUILD_DIR/$OUTPUT_FILE"
            log_info "Created compressed cpio rootfs"
        else
            mv "$BUILD_DIR/rootfs.cpio" "$BUILD_DIR/$OUTPUT_FILE"
            log_info "Created cpio rootfs (uncompressed)"
        fi
    else
        # Fallback: just create empty placeholder
        log_warn "cpio not available, creating placeholder image"
    fi
}

# Main execution
main() {
    log_info "Building c9o-mach rootfs image"
    log_info "  Type: $ROOTFS_TYPE"
    log_info "  Size: ${ROOTFS_SIZE}MB"
    log_info "  Arch: $ARCH"
    log_info "  Output: $OUTPUT_FILE"
    
    check_deps
    
    # Clean and create build directory
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    
    # Create rootfs based on type
    case "$ROOTFS_TYPE" in
        minimal)
            create_minimal_rootfs
            ;;
        full)
            create_full_rootfs
            ;;
        *)
            log_error "Unknown rootfs type: $ROOTFS_TYPE"
            exit 1
            ;;
    esac
    
    # Create the image
    create_image
    
    # Copy to final location if different
    if [[ "$BUILD_DIR/$OUTPUT_FILE" != "$OUTPUT_FILE" ]]; then
        cp "$BUILD_DIR/$OUTPUT_FILE" "$OUTPUT_FILE"
    fi
    
    log_info "Rootfs image build complete: $OUTPUT_FILE"
    ls -lh "$OUTPUT_FILE"
}

main "$@"
