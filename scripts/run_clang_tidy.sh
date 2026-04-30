#!/bin/bash
# Run clang-tidy across project sources and generate a report.

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

OUTPUT_FILE=""
SUMMARY_ONLY=false
NO_SUMMARY=false
QUIET=false
JOBS=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --output)
      OUTPUT_FILE="$2"
      shift 2
      ;;
    --summary-only)
      SUMMARY_ONLY=true
      shift
      ;;
    --no-summary)
      NO_SUMMARY=true
      shift
      ;;
    --quiet)
      QUIET=true
      shift
      ;;
    --jobs)
      JOBS="$2"
      shift 2
      ;;
    --help|-h)
      echo "Usage: $0 [options]"
      echo "  --output FILE"
      echo "  --summary-only"
      echo "  --no-summary"
      echo "  --quiet"
      echo "  --jobs N"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

if [[ -z "$JOBS" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc 2>/dev/null || echo "4")
  elif [[ "$OSTYPE" == "darwin"* ]] && command -v sysctl >/dev/null 2>&1; then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo "4")
  else
    JOBS="4"
  fi
fi

if [[ -f "$PROJECT_ROOT/scripts/clang-tidy-wrapper.sh" ]]; then
  CLANG_TIDY_CMD="$PROJECT_ROOT/scripts/clang-tidy-wrapper.sh"
elif [[ -f "/opt/homebrew/opt/llvm/bin/clang-tidy" ]]; then
  CLANG_TIDY_CMD="/opt/homebrew/opt/llvm/bin/clang-tidy"
elif [[ -f "/usr/local/opt/llvm/bin/clang-tidy" ]]; then
  CLANG_TIDY_CMD="/usr/local/opt/llvm/bin/clang-tidy"
elif command -v clang-tidy >/dev/null 2>&1; then
  CLANG_TIDY_CMD="clang-tidy"
else
  echo -e "${RED}Error: clang-tidy not found${NC}" >&2
  exit 1
fi

if [[ -f "$PROJECT_ROOT/compile_commands.json" ]]; then
  COMPILE_DB="$PROJECT_ROOT"
elif [[ -f "$PROJECT_ROOT/build/compile_commands.json" ]]; then
  COMPILE_DB="$PROJECT_ROOT/build"
elif [[ -f "$PROJECT_ROOT/build_coverage/compile_commands.json" ]]; then
  COMPILE_DB="$PROJECT_ROOT/build_coverage"
else
  echo -e "${YELLOW}Warning: compile_commands.json not found${NC}" >&2
  echo "Run: cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >&2
  exit 1
fi

if [[ -z "$OUTPUT_FILE" ]]; then
  TODAY=$(date +%Y-%m-%d)
  OUTPUT_FILE="$PROJECT_ROOT/clang-tidy-report-${TODAY}.txt"
fi

if [[ "$QUIET" == false ]]; then
  echo -e "${GREEN}=== clang-tidy Static Analysis ===${NC}"
  echo "Project: $PROJECT_ROOT"
  echo "clang-tidy: $CLANG_TIDY_CMD"
  echo "Compile DB: $COMPILE_DB"
fi

SOURCE_FILES=$(rg --files src --glob "*.cpp" --glob "*.h" | rg -v "^external/|/external/|Embedded")
FILE_COUNT=$(echo "$SOURCE_FILES" | rg -c . || echo "0")
if [[ "$FILE_COUNT" -eq 0 ]]; then
  echo -e "${RED}Error: No source files found${NC}" >&2
  exit 1
fi

TEMP_OUTPUT=$(mktemp)
trap "rm -f '$TEMP_OUTPUT'" EXIT

FILTER_SCRIPT="$PROJECT_ROOT/scripts/filter_clang_tidy_init_statements.py"
if [[ -f "$FILTER_SCRIPT" ]]; then
  echo "$SOURCE_FILES" | xargs -P "$JOBS" -n 1 "$CLANG_TIDY_CMD" -p "$COMPILE_DB" 2>&1 | python3 "$FILTER_SCRIPT" | tee "$TEMP_OUTPUT" > /dev/null || true
else
  echo "$SOURCE_FILES" | xargs -P "$JOBS" -n 1 "$CLANG_TIDY_CMD" -p "$COMPILE_DB" 2>&1 | tee "$TEMP_OUTPUT" > /dev/null || true
fi

WARNINGS=$(rg "warning:|error:" "$TEMP_OUTPUT" | rg -v "Processing file|warnings and|Error while processing" || true)
if [[ "$OSTYPE" == "darwin"* ]]; then
  WARNINGS=$(echo "$WARNINGS" | rg -v "\[clang-diagnostic-error\]" || true)
fi
WARNINGS=$(echo "$WARNINGS" | awk '/\[misc-header-include-cycle\]/ && /curl/ {next} 1' || true)

WARNING_COUNT=$(echo "$WARNINGS" | rg -c "warning:" || echo "0")
ERROR_COUNT=$(echo "$WARNINGS" | rg -c "error:" || echo "0")
TOTAL_COUNT=$((WARNING_COUNT + ERROR_COUNT))
SUMMARY_STATS=$(echo "$WARNINGS" | rg -o "\[[^]]+\]" | sort | uniq -c | sort -rn || true)

{
  echo "================================================================================"
  echo "clang-tidy Analysis Report"
  echo "Generated: $(date)"
  echo "Project: $PROJECT_ROOT"
  echo "Files Analyzed: $FILE_COUNT"
  echo "================================================================================"
  echo ""
  if [[ "$NO_SUMMARY" == false ]]; then
    echo "SUMMARY"
    echo "================================================================================"
    echo "Total Warnings: $WARNING_COUNT"
    echo "Total Errors:   $ERROR_COUNT"
    echo "Total Issues:   $TOTAL_COUNT"
    echo ""
    if [[ -n "$SUMMARY_STATS" ]]; then
      echo "Top Warning Categories:"
      echo "$SUMMARY_STATS" | head -20
      echo ""
    fi
  fi
  if [[ "$SUMMARY_ONLY" == false ]]; then
    echo "DETAILED WARNINGS AND ERRORS"
    echo "================================================================================"
    if [[ -n "$WARNINGS" ]]; then
      echo "$WARNINGS"
    else
      echo "No warnings or errors found!"
    fi
  fi
} > "$OUTPUT_FILE"

if [[ "$QUIET" == false ]]; then
  echo -e "${BLUE}Report:${NC} $OUTPUT_FILE"
fi

exit 0
