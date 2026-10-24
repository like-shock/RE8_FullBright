#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "$0")/.."
if [[ $# == 0 ]]; then
    python3 -m unittest discover -s tests -p 'test_*.py' -v
elif [[ $# == 2 && $1 == --suite ]]; then
    case "$2" in
        identity) pattern=test_build_identity.py ;;
        deps) pattern=test_deps.py ;;
        collector) pattern=test_collect_environment.py ;;
        binary) pattern=test_binary.py ;;
        *) echo "Unknown or unimplemented suite: $2" >&2; exit 2 ;;
    esac
    python3 -m unittest discover -s tests -p "$pattern" -v
else
    echo 'Usage: bash scripts/test.sh [--suite identity|deps|collector|binary]' >&2
    exit 2
fi
