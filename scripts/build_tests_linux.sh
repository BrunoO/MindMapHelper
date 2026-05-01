#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBM_ENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
