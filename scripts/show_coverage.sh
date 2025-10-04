#!/bin/bash
# Simple script to view coverage report

COVERAGE_DIR="${1:-build/atlas/coverage}"

if [ ! -f "$COVERAGE_DIR/coverage_html/index.html" ]; then
    echo "Coverage report not found at: $COVERAGE_DIR/coverage_html/index.html"
    echo ""
    echo "Run coverage first with:"
    echo "  cmake --build $COVERAGE_DIR --target coverage"
    exit 1
fi

# Try to open in browser
if command -v open &> /dev/null; then
    open "$COVERAGE_DIR/coverage_html/index.html"
elif command -v xdg-open &> /dev/null; then
    xdg-open "$COVERAGE_DIR/coverage_html/index.html"
else
    echo "Coverage report: file://$(cd "$COVERAGE_DIR" && pwd)/coverage_html/index.html"
fi
