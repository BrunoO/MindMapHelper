#!/usr/bin/env bash

# Fetch per-file code duplication details from SonarCloud Web API.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

ORGANIZATION="${SONARQUBE_ORG:-BrunoO}"
PROJECT_KEY="${SONARQUBE_PROJECT:-BrunoO_MindMapHelper}"
TOKEN="${SONARQUBE_TOKEN:-}"
BASE_URL="https://sonarcloud.io/api"
OUTPUT_DIR="${PROJECT_ROOT}/sonar-results"
OUTPUT_JSON=""
FORMAT="json"
SLEEP_SECS="0.12"

show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    --org ORGANIZATION       SonarCloud organization (default: BrunoO)
    --project PROJECT_KEY    Project key (default: BrunoO_MindMapHelper)
    --token TOKEN            SonarCloud token (or set SONARQUBE_TOKEN)
    --output-dir DIR         Directory for output (default: PROJECT_ROOT/sonar-results)
    --output FILE            JSON output path (default: DIR/sonar_duplications.json)
    --format FORMAT          json | summary (default: json)
    --sleep SECONDS          Delay between API calls (default: 0.12)
    --help                   Show this help message
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --org) ORGANIZATION="$2"; shift 2 ;;
        --project) PROJECT_KEY="$2"; shift 
            2 ;;
        --token) TOKEN="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        --output) OUTPUT_JSON="$2"; shift 2 ;;
        --format) FORMAT="$2"; shift 2 ;;
        --sleep) SLEEP_SECS="$2"; shift 2 ;;
        --help|-h) show_help ;;
        *)
            echo "ERROR: Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

if [[ -z "$TOKEN" ]]; then
    echo "ERROR: SONARQUBE_TOKEN is required (or use --token)." >&2
    exit 1
fi
if [[ ! "$FORMAT" =~ ^(json|summary)$ ]]; then
    echo "ERROR: Invalid --format (use json or summary)." >&2
    exit 1
fi
if ! command -v jq &> /dev/null; then
    echo "ERROR: jq is required." >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
if [[ -z "$OUTPUT_JSON" ]]; then
    OUTPUT_JSON="${OUTPUT_DIR}/sonar_duplications.json"
fi

KEYS_FILE=$(mktemp)
RESP_TMP=$(mktemp)
NDJSON_TMP=$(mktemp)
trap 'rm -f "$KEYS_FILE" "$RESP_TMP" "$NDJSON_TMP"' EXIT

page=1
page_size=500
while true; do
    HTTP_CODE=$(curl -s -w "%{http_code}" -o "$RESP_TMP" \
        -H "Authorization: Bearer ${TOKEN}" \
        -G "${BASE_URL}/components/tree" \
        --data-urlencode "component=${PROJECT_KEY}" \
        --data-urlencode "qualifiers=FIL" \
        --data-urlencode "ps=${page_size}" \
        --data-urlencode "p=${page}")

    if [[ "$HTTP_CODE" != "200" ]]; then
        echo "ERROR: components/tree failed (HTTP $HTTP_CODE)" >&2
        exit 1
    fi

    total=$(jq -r '.paging.total // 0' "$RESP_TMP")
    idx=$(jq -r '.paging.pageIndex // 1' "$RESP_TMP")
    ps=$(jq -r '.paging.pageSize // 500' "$RESP_TMP")
    jq -r '.components[] | [.key, .path] | @tsv' "$RESP_TMP" >> "$KEYS_FILE"

    if (( idx * ps >= total )); then
        break
    fi
    page=$((page + 1))
done

while IFS=$'\t' read -r comp_key path; do
    [[ -z "$comp_key" ]] && continue
    HTTP_CODE=$(curl -s -w "%{http_code}" -o "$RESP_TMP" \
        -H "Authorization: Bearer ${TOKEN}" \
        -G "${BASE_URL}/duplications/show" \
        --data-urlencode "key=${comp_key}")

    if [[ "$HTTP_CODE" != "200" ]]; then
        jq -n -c --arg k "$comp_key" --arg p "$path" --argjson h "$HTTP_CODE" \
            '{componentKey: $k, path: $p, error: ("HTTP " + ($h | tostring))}' >> "$NDJSON_TMP"
    else
        jq -c --arg k "$comp_key" --arg p "$path" \
            '. + {componentKey: $k, path: $p}' "$RESP_TMP" >> "$NDJSON_TMP"
    fi
    sleep "$SLEEP_SECS"
done < "$KEYS_FILE"

jq -s '.' "$NDJSON_TMP" > "$OUTPUT_JSON"
echo "Wrote: ${OUTPUT_JSON}"

if [[ "$FORMAT" == "summary" ]]; then
    jq -r '
      .[] | select(.error | not) |
      ([.duplications[]? | .blocks[]?] | length) as $blocks |
      (.duplications | length) as $groups |
      "\(.path)\tduplicate_blocks=\($blocks)\tgroups=\($groups)"
    ' "$OUTPUT_JSON"
fi
