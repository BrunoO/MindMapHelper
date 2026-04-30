#!/bin/bash
# Wrapper script for clang-tidy on macOS.
# Adds SDK include paths when Homebrew LLVM clang-tidy is used.

SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null)

if [[ -f "/opt/homebrew/opt/llvm/bin/clang-tidy" ]]; then
  CLANG_TIDY="/opt/homebrew/opt/llvm/bin/clang-tidy"
elif [[ -f "/usr/local/opt/llvm/bin/clang-tidy" ]]; then
  CLANG_TIDY="/usr/local/opt/llvm/bin/clang-tidy"
elif command -v clang-tidy >/dev/null 2>&1; then
  CLANG_TIDY="clang-tidy"
else
  echo "Error: clang-tidy not found" >&2
  exit 1
fi

EXTRA_ARGS=()
if [[ -n "$SDK_PATH" ]]; then
  EXTRA_ARGS+=("--extra-arg=-I${SDK_PATH}/usr/include/c++/v1")
  EXTRA_ARGS+=("--extra-arg=-I${SDK_PATH}/usr/include")
fi

exec "$CLANG_TIDY" "${EXTRA_ARGS[@]}" "$@"
