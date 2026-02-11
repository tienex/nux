#!/bin/bash
#
# Patch ELF Header Endianness
#
# Patches the EI_DATA field (byte 5) in an ELF file's header to mark it
# as big-endian (ELFDATA2MSB = 2). This is used for pseudo-endian kernels
# that run on little-endian hardware but expect big-endian boot structures.
#
# Usage: patch-elf-endian.sh <elf-file>

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <elf-file>" >&2
    exit 1
fi

ELF_FILE="$1"

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: File not found: $ELF_FILE" >&2
    exit 1
fi

# Check if it's actually an ELF file
if ! file "$ELF_FILE" | grep -q "ELF"; then
    echo "Error: Not an ELF file: $ELF_FILE" >&2
    exit 1
fi

# Get current endianness
CURRENT=$(od -An -t x1 -N 6 "$ELF_FILE" | tr -d ' \n' | cut -c11-12)

echo "Current EI_DATA byte: 0x$CURRENT"

if [ "$CURRENT" = "01" ]; then
    echo "  (ELFDATA2LSB - little-endian)"
elif [ "$CURRENT" = "02" ]; then
    echo "  (ELFDATA2MSB - big-endian)"
    echo "Already marked as big-endian, nothing to do."
    exit 0
else
    echo "  (Unknown value)"
fi

# Patch byte 5 to 0x02 (ELFDATA2MSB)
echo "Patching EI_DATA to 0x02 (ELFDATA2MSB - big-endian)..."

# Use printf to create a binary byte and dd to patch
printf '\x02' | dd of="$ELF_FILE" bs=1 seek=5 count=1 conv=notrunc 2>/dev/null

# Verify the change
PATCHED=$(od -An -t x1 -N 6 "$ELF_FILE" | tr -d ' \n' | cut -c11-12)

if [ "$PATCHED" = "02" ]; then
    echo "Successfully patched ELF header to big-endian."
    echo ""
    echo "Note: The binary code remains in native little-endian format."
    echo "      Only the ELF header was modified to signal the APXH"
    echo "      bootloader that boot structures should be big-endian."
else
    echo "Error: Patch failed. EI_DATA is still: 0x$PATCHED" >&2
    exit 1
fi
