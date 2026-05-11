#!/usr/bin/env bash

# Run SonarScanner CLI for SonarCloud analysis.

set -euo pipefail

ORGANIZATION="${SONARQUBE_ORG:-BrunoO}"
PROJECT_KEY="${SONARQUBE_PROJECT:-BrunoO_MindMapHelper}"
TOKEN="${SONARQUBE_TOKEN:-}"
SONAR_HOST_URL="${SONAR_HOST_URL:-https://sonarcloud.io}"
SONAR_SCANNER_PATH="${SONAR_SCANNER_PATH:-}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    --org ORGANIZATION     SonarCloud organization (default: BrunoO)
    --project PROJECT_KEY  Project key (default: BrunoO_MindMapHelper)
    --token TOKEN          SonarCloud token (or set SONARQUBE_TOKEN env var)
    --scanner-path PATH    Path to sonar-scanner binary (if not in PATH)
    --help                 Show this help message
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --org) ORGANIZATION="$2"; shift 2 ;;
        --project) PROJECT_KEY="$2"; shift 2 ;;
        --token) TOKEN="$2"; shift 2 ;;
        --scanner-path) SONAR_SCANNER_PATH="$2"; shift 2 ;;
        --help|-h) show_help ;;
        *)
            echo -e "${RED}ERROR: Unknown option: $1${NC}" >&2
            exit 1
            ;;
    esac
done

if [[ -n "$SONAR_SCANNER_PATH" ]]; then
    SONAR_SCANNER="$SONAR_SCANNER_PATH"
elif command -v sonar-scanner &> /dev/null; then
    SONAR_SCANNER="sonar-scanner"
else
    echo -e "${RED}ERROR: sonar-scanner not found in PATH${NC}" >&2
    exit 1
fi

if [[ -z "$TOKEN" ]]; then
    echo -e "${RED}ERROR: SonarCloud token is required${NC}" >&2
    echo "Set SONARQUBE_TOKEN or use --token." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

COMPILE_COMMANDS=""
if [[ -f "$PROJECT_ROOT/compile_commands.json" ]]; then
    COMPILE_COMMANDS="$PROJECT_ROOT/compile_commands.json"
elif [[ -f "$PROJECT_ROOT/build/compile_commands.json" ]]; then
    COMPILE_COMMANDS="$PROJECT_ROOT/build/compile_commands.json"
fi

SCANNER_ARGS=(
    "-Dsonar.projectKey=$PROJECT_KEY"
    "-Dsonar.organization=$ORGANIZATION"
    "-Dsonar.host.url=$SONAR_HOST_URL"
    "-Dsonar.token=$TOKEN"
)

if [[ -n "$COMPILE_COMMANDS" ]]; then
    SCANNER_ARGS+=("-Dsonar.cfamily.compile-commands=$COMPILE_COMMANDS")
else
    echo -e "${YELLOW}No compile_commands.json found; C/C++ analysis may be incomplete.${NC}"
fi

echo -e "${BLUE}Running SonarScanner...${NC}"
echo "Organization: $ORGANIZATION"
echo "Project: $PROJECT_KEY"
echo "Root: $PROJECT_ROOT"
echo ""

"$SONAR_SCANNER" "${SCANNER_ARGS[@]}"

echo ""
echo -e "${GREEN}SonarScanner analysis completed.${NC}"
echo "https://sonarcloud.io/project/overview?id=$PROJECT_KEY"
