#!/bin/sh
set -e
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

./build.sh
"$SCRIPT_DIR/build/zex-tests" "$SCRIPT_DIR/zexdoc.com"
"$SCRIPT_DIR/build/zex-tests" "$SCRIPT_DIR/zexall.com"
"$SCRIPT_DIR/build/json-tests" "$SCRIPT_DIR/zexdoc.com"
"$SCRIPT_DIR/build/Z80Assemble_test"