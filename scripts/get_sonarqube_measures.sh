#!/usr/bin/env bash

# Fetch project measures (metrics) from SonarCloud.

set -euo pipefail

ORGANIZATION="${SONARQUBE_ORG:-BrunoO}"
PROJECT_KEY="${SONARQUBE_PROJECT:-BrunoO_MindMapHelper}"
TOKEN="${SONARQUBE_TOKEN:-}"
OUTPUT_FILE="sonarqube_measures.json"
FORMAT="json"
BASE_URL="https://sonarcloud.io/api"
METRIC_KEYS="duplicated_lines_density,duplicated_lines,duplicated_blocks,ncloc,new_duplicated_lines_density,new_duplicated_lines,new_lines_to_cover,coverage,new_coverage,bugs,vulnerabilities,code_smells,sqale_rating,reliability_rating,security_rating"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    --org ORGANIZATION     SonarCloud organization (default: BrunoO)
    --project PROJECT_KEY  Project key (default: BrunoO_MindMapHelper)
    --token TOKEN          SonarCloud token (or set SONARQUBE_TOKEN env var)
    --output FILE          Output file (default: sonarqube_measures.json)
    --format FORMAT        Output format: json, summary (default: json)
    --help                 Show this help message
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --org) ORGANIZATION="$2"; shift 2 ;;
        --project) PROJECT_KEY="$2"; shift 2 ;;
        --token) TOKEN="$2"; shift 2 ;;
        --output) OUTPUT_FILE="$2"; shift 2 ;;
        --format) FORMAT="$2"; shift 2 ;;
        --help|-h) show_help ;;
        *)
            echo -e "${RED}ERROR: Unknown option: $1${NC}" >&2
            exit 1
            ;;
    esac
done

if [[ -z "$TOKEN" ]]; then
    echo -e "${RED}ERROR: SonarCloud token is required${NC}" >&2
    exit 1
fi

API_URL="${BASE_URL}/measures/component"
PARAMS="component=${PROJECT_KEY}&metricKeys=${METRIC_KEYS}"

TEMP_FILE=$(mktemp)
trap "rm -f $TEMP_FILE" EXIT

HTTP_CODE=$(curl -s -w "%{http_code}" -o "$TEMP_FILE" \
    -H "Authorization: Bearer ${TOKEN}" \
    "${API_URL}?${PARAMS}")

if [[ "$HTTP_CODE" != "200" ]]; then
    echo -e "${RED}ERROR: API request failed (HTTP $HTTP_CODE)${NC}" >&2
    exit 1
fi

if [[ "$FORMAT" == "summary" ]]; then
    if ! command -v jq &> /dev/null; then
        echo -e "${RED}ERROR: jq is required for summary format${NC}" >&2
        exit 1
    fi
    jq -r '.component.measures[] | "\(.metric): \(.value)"' "$TEMP_FILE"
fi

if command -v jq &> /dev/null; then
    jq '.' "$TEMP_FILE" > "$OUTPUT_FILE"
else
    cp "$TEMP_FILE" "$OUTPUT_FILE"
fi

echo -e "${BLUE}Project: $PROJECT_KEY${NC}"
echo -e "${GREEN}Measures saved to: $OUTPUT_FILE${NC}"
