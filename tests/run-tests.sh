#!/bin/sh
set -e

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/zex-tests zexdoc.com
./build/zex-tests zexall.com
./build/json-tests zexdoc.com