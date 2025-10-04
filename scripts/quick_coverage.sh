#!/bin/bash
# Quick coverage check using existing build
# This works if you already built with --coverage flags

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

BUILD_DIR="${1:-build/atlas/debug}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory $BUILD_DIR not found"
    exit 1
fi

cd "$BUILD_DIR"

echo -e "${GREEN}Quick Coverage Check${NC}"
echo "===================="
echo

# Run tests first
echo -e "${YELLOW}Running tests...${NC}"
ctest --output-on-fail --quiet

echo
echo -e "${YELLOW}Coverage Summary:${NC}"
echo

# Simple coverage report for source files
for src in $(find . -name "*.gcno" | grep -v test | grep -v _deps | sed 's/\.gcno$//' | sort -u); do
    if [ -f "${src}.gcda" ]; then
        gcov "${src}.gcda" -o "$(dirname $src)" 2>/dev/null | grep -A 2 "^File.*src/" | head -3
    fi
done

echo
echo -e "${GREEN}Done!${NC}"
echo
echo "For detailed HTML report, run: ./run_coverage.sh"
