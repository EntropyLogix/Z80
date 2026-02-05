#!/bin/sh
set -e
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

if [ "$1" = "--coverage" ]; then
    "$SCRIPT_DIR/build.sh" --coverage
else
    "$SCRIPT_DIR/build.sh"
fi

"$SCRIPT_DIR/build/tests/zex-tests" "$SCRIPT_DIR/zexdoc.com"
"$SCRIPT_DIR/build/tests/zex-tests" "$SCRIPT_DIR/zexall.com"
"$SCRIPT_DIR/build/tests/json-tests" "$SCRIPT_DIR/zexdoc.com"
"$SCRIPT_DIR/build/tests/Assembler_test"
"$SCRIPT_DIR/build/tests/Decoder_test"
"$SCRIPT_DIR/build/tests/CPU_test"

if [ "$1" = "--coverage" ]; then
    if command -v lcov >/dev/null 2>&1; then
        echo "--- Generating coverage report ---"
        lcov --capture --directory "$SCRIPT_DIR/build" --output-file "$SCRIPT_DIR/build/coverage.info" --ignore-errors mismatch
        # Remove system files and test files themselves from the report to focus on source code
        lcov --remove "$SCRIPT_DIR/build/coverage.info" '/usr/*' "${SCRIPT_DIR}/*" '*tests/*' '*_test.cpp' --output-file "$SCRIPT_DIR/build/coverage.info" --ignore-errors unused
        genhtml "$SCRIPT_DIR/build/coverage.info" --output-directory "$SCRIPT_DIR/coverage_report"
        echo "Report generated at: $SCRIPT_DIR/coverage_report/index.html"
    else
        echo "Warning: lcov not found. Coverage report skipped."
    fi
fi