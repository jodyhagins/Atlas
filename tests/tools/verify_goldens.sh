#!/bin/bash
# Verify golden files match current Atlas output

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ATLAS_BIN="${ATLAS_BIN:-${HOME}/build/atlas/debug/bin/atlas}"
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

# Verify .input files
while IFS= read -r -d '' input_file; do
    expected_file="${input_file%.input}.expected"

    if [[ ! -f "$expected_file" ]]; then
        echo "MISSING: $expected_file"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Check if this is an interactions test by examining the path
    if [[ "${input_file}" =~ "/interactions/" ]]; then
        interactions_flag="--interactions=true"
    else
        interactions_flag=""
    fi

    # Generate current output
    actual=$("$ATLAS_BIN" $interactions_flag --input="$input_file")
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

# Verify .cmdline files
while IFS= read -r -d '' cmdline_file; do
    expected_file="${cmdline_file%.cmdline}.expected"

    if [[ ! -f "$expected_file" ]]; then
        echo "MISSING: $expected_file"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Read command-line arguments from file (one per line, skip comments and empty lines)
    args=()
    while IFS= read -r line; do
        # Trim whitespace
        line=$(echo "$line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')
        # Skip empty lines and comments
        if [[ -n "$line" && ! "$line" =~ ^# ]]; then
            args+=("$line")
        fi
    done < "$cmdline_file"

    # Generate current output
    actual=$("$ATLAS_BIN" "${args[@]}")
    expected=$(cat "$expected_file")

    if [[ "$actual" != "$expected" ]]; then
        echo "FAIL: $(basename "$cmdline_file")"
        echo "  Run: ./tests/tools/update_goldens.sh"
        FAILED=$((FAILED + 1))
    else
        echo "PASS: $(basename "$cmdline_file")"
        PASSED=$((PASSED + 1))
    fi
done < <(find "$GOLDEN_DIR" -name "*.cmdline" -print0 | sort -z)

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
