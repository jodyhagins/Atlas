#!/bin/bash
# Update all golden files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ATLAS_BIN="${ATLAS_BIN:-${HOME}/build/atlas/debug/bin/atlas}"
GOLDEN_DIR="$REPO_ROOT/tests/fixtures/golden"

if [[ ! -f "$ATLAS_BIN" ]]; then
    echo "Error: Atlas binary not found at $ATLAS_BIN"
    echo "Build Atlas first, or set ATLAS_BIN environment variable"
    echo "  Example: cmake --build ${HOME}/build/atlas/debug"
    exit 1
fi

echo "Updating golden files..."
echo "Atlas: $ATLAS_BIN"
echo "Golden dir: $GOLDEN_DIR"
echo ""

UPDATED=0

while IFS= read -r -d '' input_file; do
    expected_file="${input_file%.input}.expected"

    echo "Updating: $(basename "$input_file")"

    if [[ "${input_file}" =~ "/interactions/" ]]; then
        interactions=" --interactions=true"
    else
        interactions=""
    fi

    # Generate code
    echo "+++++ $ATLAS_BIN" ${interactions} --input="$input_file"
    "$ATLAS_BIN" ${interactions} --input="$input_file" > "$expected_file"

    UPDATED=$((UPDATED + 1))
done < <(find "$GOLDEN_DIR" -name "*.input" -print0 | sort -z)

echo ""
echo "Updated $UPDATED golden file(s)"
echo ""
echo "Review changes:"
echo "  cd $GOLDEN_DIR && git diff"
echo ""
echo "If changes are intentional:"
echo "  git add tests/fixtures/golden/"
