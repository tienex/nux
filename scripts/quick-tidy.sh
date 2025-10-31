#!/bin/bash
# Quick clang-tidy check for recently modified files
# Usage: ./scripts/quick-tidy.sh [--fix]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

FIX_MODE="${1:-}"

# Get list of modified/staged files from git
echo "Checking git for modified files..."
MODIFIED_FILES=$(git diff --name-only --diff-filter=ACMRTUXB HEAD | grep -E '\.(c|h)$' || true)
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACMRTUXB | grep -E '\.(c|h)$' || true)

# Combine and deduplicate
ALL_FILES=$(echo -e "${MODIFIED_FILES}\n${STAGED_FILES}" | sort -u | grep -v '^$' || true)

if [ -z "${ALL_FILES}" ]; then
    echo "No modified C/H files found."
    exit 0
fi

echo "Found modified files:"
echo "${ALL_FILES}"
echo ""

# Run clang-tidy on these files
if [ "${FIX_MODE}" = "--fix" ]; then
    exec "${SCRIPT_DIR}/run-clang-tidy.sh" "${PROJECT_ROOT}" --fix
else
    exec "${SCRIPT_DIR}/run-clang-tidy.sh" "${PROJECT_ROOT}"
fi
