#!/bin/sh
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT="$SCRIPT_DIR/.."

if [ "$1" = "--optimized" ]; then
  echo "--- Running optimized build (PGO) ---"
  rm -rf "$PROJECT_ROOT/tests/build"
  cmake -B "$PROJECT_ROOT/tests/build" -S "$PROJECT_ROOT/tests/" -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=GENERATE
  cmake --build "$PROJECT_ROOT/tests/build" --config Release
  "$PROJECT_ROOT/tests/build/zex-tests" "$PROJECT_ROOT/tests/zexdoc.com"
  cmake -B "$PROJECT_ROOT/tests/build" -S "$PROJECT_ROOT/tests/" -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=USE
  cmake --build "$PROJECT_ROOT/tests/build" --config Release
else
  echo "--- Running standard release build ---"
  cmake -B "$PROJECT_ROOT/tests/build" -S "$PROJECT_ROOT/tests/" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$PROJECT_ROOT/tests/build" --config Release
fi