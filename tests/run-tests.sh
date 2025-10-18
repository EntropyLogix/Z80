#!/bin/sh
set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

cmake -B "$SCRIPT_DIR/build" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$SCRIPT_DIR/build" --config Release
"$SCRIPT_DIR/build/zex-tests" "$SCRIPT_DIR/zexdoc.com"
"$SCRIPT_DIR/build/zex-tests" "$SCRIPT_DIR/zexall.com"
"$SCRIPT_DIR/build/json-tests" "$SCRIPT_DIR/zexdoc.com"