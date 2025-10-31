#!/bin/bash
# Run clang-tidy on NUX kernel source files
# Usage:
#   ./scripts/run-clang-tidy.sh [path]           # Check files
#   ./scripts/run-clang-tidy.sh [path] --fix     # Check and auto-fix files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CLANG_TIDY_CONFIG="${PROJECT_ROOT}/.clang-tidy"

# Default target is all source files
TARGET_PATH="${1:-${PROJECT_ROOT}}"
FIX_MODE="${2:-}"

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo -e "${RED}Error: clang-tidy not found. Please install it.${NC}"
    echo "  Ubuntu/Debian: apt-get install clang-tidy"
    exit 1
fi

# Check if .clang-tidy config exists
if [ ! -f "${CLANG_TIDY_CONFIG}" ]; then
    echo -e "${RED}Error: .clang-tidy configuration not found at ${CLANG_TIDY_CONFIG}${NC}"
    exit 1
fi

echo -e "${GREEN}Running clang-tidy on NUX kernel${NC}"
echo "Target: ${TARGET_PATH}"
echo "Config: ${CLANG_TIDY_CONFIG}"
echo ""

# Determine if we're in fix mode
FIX_ARGS=""
if [ "${FIX_MODE}" = "--fix" ] || [ "${FIX_MODE}" = "-fix" ]; then
    echo -e "${YELLOW}Fix mode enabled - will apply automatic fixes${NC}"
    FIX_ARGS="--fix --fix-errors"
else
    echo -e "${YELLOW}Check mode - no automatic fixes will be applied${NC}"
    echo "To enable auto-fix, run: $0 ${TARGET_PATH} --fix"
fi
echo ""

# Find all .c and .h files
echo "Finding source files..."
if [ -f "${TARGET_PATH}" ]; then
    # Single file specified
    FILES="${TARGET_PATH}"
else
    # Directory specified - find all C files
    FILES=$(find "${TARGET_PATH}" -type f \( -name "*.c" -o -name "*.h" \) \
        ! -path "*/build/*" \
        ! -path "*/out/*" \
        ! -path "*/.git/*" \
        ! -path "*/test_*")
fi

FILE_COUNT=$(echo "${FILES}" | wc -l)
echo -e "Found ${GREEN}${FILE_COUNT}${NC} files to check"
echo ""

# Compilation flags for kernel code
COMPILE_FLAGS="-I${PROJECT_ROOT} \
    -I${PROJECT_ROOT}/ananke/include \
    -I${PROJECT_ROOT}/nux/libs/ecrt/include/public \
    -I${PROJECT_ROOT}/nux/libs/hal/include/public \
    -I${PROJECT_ROOT}/nux/libs/nux/include/public \
    -I${PROJECT_ROOT}/platform/include/public \
    -D__KERNEL__ \
    -ffreestanding \
    -fno-builtin \
    -nostdinc"

# Run clang-tidy on each file
TOTAL=0
PASSED=0
FAILED=0

for file in ${FILES}; do
    TOTAL=$((TOTAL + 1))
    echo -e "${YELLOW}[$TOTAL/$FILE_COUNT]${NC} Checking ${file}..."

    if clang-tidy "${file}" ${FIX_ARGS} -- ${COMPILE_FLAGS} 2>&1 | tee /tmp/clang-tidy.log | grep -q "warning:"; then
        FAILED=$((FAILED + 1))
        echo -e "  ${RED}✗ Warnings found${NC}"
    else
        PASSED=$((PASSED + 1))
        echo -e "  ${GREEN}✓ Passed${NC}"
    fi
    echo ""
done

# Summary
echo ""
echo "========================================="
echo "Summary"
echo "========================================="
echo -e "Total files: ${TOTAL}"
echo -e "${GREEN}Passed: ${PASSED}${NC}"
echo -e "${RED}Failed: ${FAILED}${NC}"
echo ""

if [ ${FAILED} -eq 0 ]; then
    echo -e "${GREEN}All checks passed!${NC}"
    exit 0
else
    echo -e "${RED}Some checks failed. Review the warnings above.${NC}"
    if [ "${FIX_MODE}" != "--fix" ]; then
        echo -e "${YELLOW}Tip: Run with --fix to automatically apply fixes${NC}"
    fi
    exit 1
fi
