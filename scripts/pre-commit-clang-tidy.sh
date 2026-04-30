#!/bin/bash
#
# Pre-commit hook to run clang-tidy on staged C++ files.
# Usage:
#   cp scripts/pre-commit-clang-tidy.sh .git/hooks/pre-commit
#   chmod +x .git/hooks/pre-commit

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PROJECT_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
cd "$PROJECT_ROOT"

if [[ -f "$PROJECT_ROOT/scripts/clang-tidy-wrapper.sh" ]]; then
  CLANG_TIDY_CMD="$PROJECT_ROOT/scripts/clang-tidy-wrapper.sh"
elif [[ -f "/opt/homebrew/opt/llvm/bin/clang-tidy" ]]; then
  CLANG_TIDY_CMD="/opt/homebrew/opt/llvm/bin/clang-tidy"
elif [[ -f "/usr/local/opt/llvm/bin/clang-tidy" ]]; then
  CLANG_TIDY_CMD="/usr/local/opt/llvm/bin/clang-tidy"
elif command -v clang-tidy &> /dev/null; then
  CLANG_TIDY_CMD="clang-tidy"
else
  echo -e "${YELLOW}Warning: clang-tidy not found. Skipping pre-commit checks.${NC}"
  exit 0
fi

if [ -f "${PROJECT_ROOT}/compile_commands.json" ]; then
  BUILD_DIR="${PROJECT_ROOT}"
elif [ -f "${PROJECT_ROOT}/build/compile_commands.json" ]; then
  BUILD_DIR="${PROJECT_ROOT}/build"
elif [ -f "${PROJECT_ROOT}/build_coverage/compile_commands.json" ]; then
  BUILD_DIR="${PROJECT_ROOT}/build_coverage"
else
  echo -e "${YELLOW}Warning: compile_commands.json not found.${NC}"
  echo "Run: cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
  echo "Skipping clang-tidy checks for this commit."
  exit 0
fi

if [ ! -f "${PROJECT_ROOT}/.clang-tidy" ]; then
  echo -e "${YELLOW}Warning: .clang-tidy not found. Skipping pre-commit checks.${NC}"
  exit 0
fi

STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h|hpp|cxx|cc)$' || true)

if [ -z "$STAGED_FILES" ]; then
  exit 0
fi

echo "Running clang-tidy on staged C++ files..."
echo ""

ISSUES_FOUND=0

for FILE in $STAGED_FILES; do
  if [[ "$FILE" == external/* ]]; then
    continue
  fi

  if [ ! -f "$FILE" ]; then
    continue
  fi

  echo "Checking $FILE..."

  # clang-tidy returns non-zero on diagnostics; capture output and decide below.
  FILTER_SCRIPT="${PROJECT_ROOT}/scripts/filter_clang_tidy_init_statements.py"
  if [[ -f "$FILTER_SCRIPT" ]]; then
    CLANG_OUTPUT=$($CLANG_TIDY_CMD -p "$BUILD_DIR" "$FILE" --quiet 2>&1 | python3 "$FILTER_SCRIPT" || true)
  else
    CLANG_OUTPUT=$($CLANG_TIDY_CMD -p "$BUILD_DIR" "$FILE" --quiet 2>&1 || true)
  fi

  echo "$CLANG_OUTPUT" | \
    grep -v "llvmlibc-" | \
    grep -E "(readability-non-const-parameter|warning:|error:)" > /tmp/clang-tidy-output.txt || true

  if [[ "$OSTYPE" == "darwin"* ]]; then
    grep -v "\[clang-diagnostic-error\]" /tmp/clang-tidy-output.txt > /tmp/clang-tidy-filtered.txt || true
    mv /tmp/clang-tidy-filtered.txt /tmp/clang-tidy-output.txt
  fi

  awk '/\[misc-header-include-cycle\]/ && /curl/ {next} 1' /tmp/clang-tidy-output.txt > /tmp/clang-tidy-filtered.txt || true
  mv /tmp/clang-tidy-filtered.txt /tmp/clang-tidy-output.txt

  if [[ -s /tmp/clang-tidy-output.txt ]]; then
    echo -e "${RED}Issues found in $FILE:${NC}"
    echo "$CLANG_OUTPUT"
    echo ""
    ISSUES_FOUND=1
  fi
done

rm -f /tmp/clang-tidy-output.txt /tmp/clang-tidy-filtered.txt

if [ $ISSUES_FOUND -eq 1 ]; then
  echo -e "${RED}clang-tidy found issues in staged files.${NC}"
  echo "Please fix issues before committing."
  echo "To bypass (not recommended): git commit --no-verify"
  exit 1
fi

echo -e "${GREEN}All clang-tidy checks passed!${NC}"
exit 0
