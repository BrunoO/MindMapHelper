#!/usr/bin/env bash

# Fetch SonarCloud issues via API (entire codebase by default).
# Usage: ./fetch_sonar_results.sh [OPTIONS]

set -euo pipefail

ORGANIZATION="${SONARQUBE_ORG:-BrunoO}"
PROJECT_KEY="${SONARQUBE_PROJECT:-BrunoO_MindMapHelper}"
TOKEN="${SONARQUBE_TOKEN:-}"
FORMAT="json"
OUTPUT_DIR="./sonar-results"
OUTPUT_FILE=""
OPEN_ONLY=false
NEW_CODE_ONLY=false
SEVERITIES=""
TYPES=""
BASE_URL="https://sonarcloud.io/api"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Fetch SonarCloud issues via API. Default scope is entire codebase (not new code only).

Options:
    --org ORGANIZATION     SonarCloud organization (default: BrunoO)
    --project PROJECT_KEY  Project key (default: BrunoO_MindMapHelper)
    --token TOKEN          SonarCloud token (or set SONARQUBE_TOKEN env var)
    --format FORMAT        Output format: json, csv, summary (default: json)
    --output-dir DIR       Directory for output file (default: ./sonar-results)
    --output FILE          Override output file path
    --open-only            Fetch only open/unresolved issues (default: all issues)
    --new-code-only        Fetch only issues in new code (default: entire codebase)
    --severities SEVS      Comma-separated: BLOCKER,HIGH,MEDIUM,LOW,INFO
    --types TYPES          Comma-separated: BUG,VULNERABILITY,CODE_SMELL,SECURITY_HOTSPOT
    --help                 Show this help message

Environment Variables:
    SONARQUBE_TOKEN        SonarCloud authentication token (required)
    SONARQUBE_ORG          SonarCloud organization (default: BrunoO)
    SONARQUBE_PROJECT      Project key (default: BrunoO_MindMapHelper)
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --org) ORGANIZATION="$2"; shift 2 ;;
        --project) PROJECT_KEY="$2"; shift 2 ;;
        --token) TOKEN="$2"; shift 2 ;;
        --format) FORMAT="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        --output) OUTPUT_FILE="$2"; shift 2 ;;
        --open-only) OPEN_ONLY=true; shift ;;
        --new-code-only) NEW_CODE_ONLY=true; shift ;;
        --severities) SEVERITIES="$2"; shift 2 ;;
        --types) TYPES="$2"; shift 2 ;;
        --help|-h) show_help ;;
        *)
            echo -e "${RED}ERROR: Unknown option: $1${NC}" >&2
            echo "Use --help for usage." >&2
            exit 1
            ;;
    esac
done

if [[ -z "$TOKEN" ]]; then
    echo -e "${RED}ERROR: SonarCloud token is required${NC}" >&2
    echo "Set SONARQUBE_TOKEN or use --token." >&2
    exit 1
fi

if [[ ! "$FORMAT" =~ ^(json|csv|summary)$ ]]; then
    echo -e "${RED}ERROR: Invalid format: $FORMAT${NC}" >&2
    exit 1
fi

if [[ -z "$OUTPUT_FILE" ]]; then
    mkdir -p "$OUTPUT_DIR"
    case "$FORMAT" in
        json) OUTPUT_FILE="$OUTPUT_DIR/sonarqube_issues.json" ;;
        csv) OUTPUT_FILE="$OUTPUT_DIR/sonarqube_issues.csv" ;;
        summary) OUTPUT_FILE="$OUTPUT_DIR/sonarqube_issues_summary.txt" ;;
    esac
fi

echo -e "${BLUE}Fetching SonarCloud results...${NC}"
echo "Organization: $ORGANIZATION"
echo "Project: $PROJECT_KEY"
echo "Format: $FORMAT"
echo "Output: $OUTPUT_FILE"
echo ""

PARAMS=(
    "organization=${ORGANIZATION}"
    "projects=${PROJECT_KEY}"
    "ps=500"
    "p=1"
)
[[ "$NEW_CODE_ONLY" == "true" ]] && PARAMS+=("inNewCodePeriod=true")
[[ -n "$SEVERITIES" ]] && PARAMS+=("severities=${SEVERITIES}")
[[ -n "$TYPES" ]] && PARAMS+=("types=${TYPES}")
[[ "$OPEN_ONLY" == "true" ]] && PARAMS+=("issueStatuses=OPEN,CONFIRMED")

QUERY_STRING=$(IFS='&'; echo "${PARAMS[*]}")
API_URL="${BASE_URL}/issues/search"

TEMP_FILE=$(mktemp)
trap 'rm -f "$TEMP_FILE"' EXIT

PAGE=1
TOTAL_ISSUES=0
ALL_ISSUES="[]"

while true; do
    echo -e "${YELLOW}Fetching page $PAGE...${NC}"
    QUERY_STRING=$(echo "$QUERY_STRING" | sed "s/p=[0-9]*/p=$PAGE/")

    HTTP_CODE=$(curl -s -w "%{http_code}" -o "$TEMP_FILE" \
        -H "Authorization: Bearer ${TOKEN}" \
        "${API_URL}?${QUERY_STRING}")

    if [[ "$HTTP_CODE" != "200" ]]; then
        echo -e "${RED}ERROR: API request failed (HTTP $HTTP_CODE)${NC}" >&2
        exit 1
    fi

    if ! command -v jq &> /dev/null; then
        echo -e "${RED}ERROR: jq is required. Install with: brew install jq${NC}" >&2
        exit 1
    fi

    PAGE_ISSUES=$(jq -c '.issues' "$TEMP_FILE")
    TOTAL=$(jq -r '.total' "$TEMP_FILE")
    PAGES=$(jq -r '.paging.total' "$TEMP_FILE" 2>/dev/null || jq -r '.total' "$TEMP_FILE")
    PS=$(jq -r '.paging.pageSize' "$TEMP_FILE" 2>/dev/null || echo "500")

    if [[ -z "$PAGES" ]] || [[ "$PAGES" == "null" ]]; then
        if [[ -n "$TOTAL" ]] && [[ "$TOTAL" != "null" ]] && [[ "$TOTAL" -gt 0 ]]; then
            PAGES=$(( (TOTAL + PS - 1) / PS ))
        else
            PAGES=1
        fi
    fi

    if [[ "$PAGE_ISSUES" == "[]" ]] || [[ -z "$PAGE_ISSUES" ]]; then
        break
    fi

    ALL_ISSUES=$(echo "$ALL_ISSUES" | jq -c ". + $PAGE_ISSUES")
    ISSUE_COUNT=$(echo "$PAGE_ISSUES" | jq 'length')
    TOTAL_ISSUES=$((TOTAL_ISSUES + ISSUE_COUNT))

    if [[ "$PAGE" -ge "$PAGES" ]]; then
        break
    fi
    PAGE=$((PAGE + 1))
done

echo -e "${GREEN}Total issues fetched: $TOTAL_ISSUES${NC}"

case "$FORMAT" in
    json)
        echo "$ALL_ISSUES" | jq '.' > "$OUTPUT_FILE"
        ;;
    csv)
        echo "key,severity,type,status,component,line,message,rule" > "$OUTPUT_FILE"
        echo "$ALL_ISSUES" | jq -r '.[] | [
            .key,
            .severity,
            .type,
            .status,
            .component,
            .line // "N/A",
            .message,
            .rule
        ] | @csv' >> "$OUTPUT_FILE"
        ;;
    summary)
        {
            echo "SonarCloud Issues Summary"
            echo "========================="
            echo "Project: $PROJECT_KEY"
            echo "Organization: $ORGANIZATION"
            echo "Total Issues: $TOTAL_ISSUES"
        } > "$OUTPUT_FILE"
        ;;
esac

echo -e "${GREEN}Results saved to: $OUTPUT_FILE${NC}"
