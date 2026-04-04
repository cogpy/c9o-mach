#!/bin/bash
# Kernel Stack Analysis Tool
# Inspired by: lustre's checkstack utility
#
# Analyzes compiled kernel objects to find functions with excessive stack usage.
# Microkernel stack space is limited (~4-8KB), so monitoring is critical.
#
# Usage: ./scripts/checkstack.sh [object-file|directory] [threshold]
# Default threshold: 512 bytes

set -euo pipefail

THRESHOLD=${2:-512}
TARGET="${1:-.}"
ARCH="${ARCH:-i386}"

echo "=== Kernel Stack Usage Analysis ==="
echo "Architecture: $ARCH"
echo "Threshold: ${THRESHOLD} bytes"
echo "Target: $TARGET"
echo ""

# Find object files
if [ -d "$TARGET" ]; then
    OBJECTS=$(find "$TARGET" -name '*.o' -not -path '*/build-*' 2>/dev/null)
else
    OBJECTS="$TARGET"
fi

if [ -z "$OBJECTS" ]; then
    echo "No object files found in $TARGET"
    exit 0
fi

VIOLATIONS=0
TOTAL=0

analyze_object() {
    local obj="$1"

    # Use objdump to disassemble and find stack frame setup
    # Look for 'sub $N,%esp' (i386) or 'sub $N,%rsp' (x86_64)
    case "$ARCH" in
        i386|i686)
            PATTERN='sub[[:space:]]+\$0x([0-9a-f]+),%esp'
            ;;
        x86_64)
            PATTERN='sub[[:space:]]+\$0x([0-9a-f]+),%rsp'
            ;;
        *)
            echo "Unsupported architecture: $ARCH"
            exit 1
            ;;
    esac

    local current_func=""
    while IFS= read -r line; do
        # Track current function name
        if echo "$line" | grep -qE '^[0-9a-f]+ <[^>]+>:'; then
            current_func=$(echo "$line" | sed 's/.*<\(.*\)>:/\1/')
        fi

        # Check for stack allocation
        if echo "$line" | grep -qiE "$PATTERN"; then
            local hex_size
            hex_size=$(echo "$line" | grep -oiE "$PATTERN" | grep -oiE '0x[0-9a-f]+' | head -1)
            local dec_size=$((hex_size))

            ((TOTAL++))

            if [ "$dec_size" -ge "$THRESHOLD" ]; then
                printf "  %-50s %6d bytes  %s\n" "$current_func" "$dec_size" "$obj"
                ((VIOLATIONS++))
            fi
        fi
    done < <(objdump -d "$obj" 2>/dev/null || true)
}

echo "Functions exceeding ${THRESHOLD}-byte stack threshold:"
echo "$(printf '%.0s-' {1..80})"

for obj in $OBJECTS; do
    analyze_object "$obj"
done

echo ""
echo "$(printf '%.0s-' {1..80})"
echo "Summary:"
echo "  Functions analyzed: $TOTAL"
echo "  Stack violations (>=${THRESHOLD}B): $VIOLATIONS"

if [ "$VIOLATIONS" -gt 0 ]; then
    echo ""
    echo "WARNING: $VIOLATIONS functions exceed the stack threshold."
    echo "Consider reducing stack usage or using heap allocation."
    exit 1
fi

echo "All functions within stack budget."
