#!/bin/bash
# Generate showcase examples to demonstrate atlas features

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/../build/atlas/debug}"
ATLAS="$BUILD_DIR/bin/atlas"

if [ ! -x "$ATLAS" ]; then
    echo "Error: Atlas executable not found at $ATLAS"
    echo "Build the project first or set BUILD_DIR environment variable"
    exit 1
fi

OUTPUT_DIR="$SCRIPT_DIR/generated"
mkdir -p "$OUTPUT_DIR"

echo "Generating strong types showcase..."
"$ATLAS" \
    --input="$SCRIPT_DIR/strong_types_showcase.txt" \
    --output="$OUTPUT_DIR/strong_types_showcase.hpp"

echo "Generating interactions showcase..."
"$ATLAS" \
    --interactions=true \
    --input="$SCRIPT_DIR/interactions_showcase.txt" \
    --output="$OUTPUT_DIR/interactions_showcase.hpp"

echo ""
echo "Generated files in $OUTPUT_DIR:"
ls -lh "$OUTPUT_DIR"

echo ""
echo "To view the generated strong types:"
echo "  less $OUTPUT_DIR/strong_types_showcase.hpp"
echo ""
echo "To view the generated interactions:"
echo "  less $OUTPUT_DIR/interactions_showcase.hpp"
