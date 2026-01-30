#!/bin/sh
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/build"

if [ "$1" = "--optimized" ]; then
  echo "--- Running optimized build (PGO) ---"
  rm -rf "$BUILD_DIR"
  echo "--- PGO: Generating profile data... ---"
  cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=GENERATE
  cmake --build "$BUILD_DIR" --config Release
  "$BUILD_DIR/zex-tests" "$SCRIPT_DIR/zexdoc.com"
  echo "--- PGO: Using profile data for optimization... ---"
  cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=USE
  cmake --build "$BUILD_DIR" --config Release
elif [ "$1" = "--coverage" ]; then
  echo "--- Running build with Code Coverage ---"
  rm -rf "$BUILD_DIR"
  cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=ON
  cmake --build "$BUILD_DIR" --config Release
else
  echo "--- Running standard release build ---"
  cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$BUILD_DIR" --config Release
fi
