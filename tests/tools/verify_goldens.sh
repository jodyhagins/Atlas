#!/bin/bash
# Verify golden files match current Atlas output

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ATLAS_BIN="${ATLAS_BIN:-/Users/jhagins/build/atlas/debug/bin/atlas}"
GOLDEN_DIR="$REPO_ROOT/tests/fixtures/golden"

if [[ ! -f "$ATLAS_BIN" ]]; then
    echo "Error: Atlas binary not found at $ATLAS_BIN"
    echo "Build Atlas first, or set ATLAS_BIN environment variable"
    exit 1
fi

echo "Verifying golden files..."
echo "Atlas: $ATLAS_BIN"
echo "Golden dir: $GOLDEN_DIR"
echo ""

FAILED=0
PASSED=0

while IFS= read -r -d '' input_file; do
    expected_file="${input_file%.input}.expected"

    if [[ ! -f "$expected_file" ]]; then
        echo "MISSING: $expected_file"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Generate current output
    actual=$("$ATLAS_BIN" --input="$input_file")
    expected=$(cat "$expected_file")

    if [[ "$actual" != "$expected" ]]; then
        echo "FAIL: $(basename "$input_file")"
        echo "  Run: diff <($ATLAS_BIN --input=\"$input_file\") \"$expected_file\""
        FAILED=$((FAILED + 1))
    else
        echo "PASS: $(basename "$input_file")"
        PASSED=$((PASSED + 1))
    fi
done < <(find "$GOLDEN_DIR" -name "*.input" -print0 | sort -z)

echo ""
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [[ $FAILED -gt 0 ]]; then
    echo ""
    echo "Golden files don't match current output!"
    echo "If changes are intentional, run: ./tests/tools/update_goldens.sh"
    exit 1
fi

echo ""
echo "All golden files match ✓"
