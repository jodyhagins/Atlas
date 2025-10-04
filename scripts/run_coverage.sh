#!/bin/bash
# Coverage analysis script for Atlas project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Atlas Code Coverage Analysis${NC}"
echo "================================"
echo

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: Must run from project root${NC}"
    exit 1
fi

# Determine build directory
if [ -d "build/coverage" ]; then
    BUILD_DIR="build/coverage"
else
    BUILD_DIR="build/atlas/coverage"
fi

echo -e "${YELLOW}Step 1: Configuring build with coverage enabled...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake ../.. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"

echo
echo -e "${YELLOW}Step 2: Building project...${NC}"
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo
echo -e "${YELLOW}Step 3: Running tests...${NC}"
ctest --output-on-failure

echo
echo -e "${YELLOW}Step 4: Collecting coverage data...${NC}"

# Find all .gcda files and run gcov on them
find . -name "*.gcda" -type f | while read gcda; do
    dir=$(dirname "$gcda")
    base=$(basename "$gcda" .gcda)
    (cd "$dir" && gcov "$base.gcda" > /dev/null 2>&1)
done

echo
echo -e "${YELLOW}Step 5: Generating coverage report...${NC}"

# Use lcov if available, otherwise use simple gcov report
if command -v lcov &> /dev/null; then
    echo "Using lcov for detailed HTML report..."

    # Capture coverage data
    lcov --capture --directory . --output-file coverage.info

    # Remove external/test files from coverage
    lcov --remove coverage.info \
        '/usr/*' \
        '*/test/*' \
        '*/tests/*' \
        '*/_deps/*' \
        '*/doctest.hpp' \
        --output-file coverage_filtered.info

    # Generate HTML report
    genhtml coverage_filtered.info --output-directory coverage_html

    echo
    echo -e "${GREEN}Coverage report generated!${NC}"
    echo
    echo "HTML report: file://$PWD/coverage_html/index.html"
    echo

    # Try to open in browser
    if command -v open &> /dev/null; then
        echo -e "${YELLOW}Opening report in browser...${NC}"
        open coverage_html/index.html
    elif command -v xdg-open &> /dev/null; then
        xdg-open coverage_html/index.html
    fi
else
    echo "lcov not found, generating simple text report..."
    echo
    echo "=== Coverage Summary ==="

    # Find all .gcov files and summarize
    for gcov_file in $(find . -name "*.gcov" | grep -v "/_deps/" | grep -v "/test"); do
        if [ -f "$gcov_file" ]; then
            lines=$(grep -E "^\s+[0-9]+:" "$gcov_file" | wc -l)
            covered=$(grep -E "^\s+[1-9][0-9]*:" "$gcov_file" | wc -l)
            if [ "$lines" -gt 0 ]; then
                percent=$((covered * 100 / lines))
                filename=$(basename "$gcov_file" .gcov)
                printf "%-50s %3d%% (%d/%d lines)\n" "$filename" "$percent" "$covered" "$lines"
            fi
        fi
    done

    echo
    echo -e "${YELLOW}To install lcov for HTML reports:${NC}"
    echo "  macOS:  brew install lcov"
    echo "  Ubuntu: sudo apt-get install lcov"
fi

echo
echo -e "${GREEN}Done!${NC}"
