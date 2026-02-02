#!/bin/sh
set -e

# This script builds the command-line tools (Z80Dump, Z80Asm).

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$SCRIPT_DIR/build" # Budujemy w tools/build

echo "--- Building Z80 tools (Release mode) ---"

cmake -B "$BUILD_DIR" -S "$PROJECT_ROOT" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release

echo "--- Build complete ---"
echo "Tools are located in: $BUILD_DIR/tools"

echo ""
echo "--- Running tests ---"
cd "$SCRIPT_DIR/../tests"
./run-tests.sh