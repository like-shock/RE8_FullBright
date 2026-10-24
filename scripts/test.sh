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
        profile) pattern=test_profile.py ;;
        runtime) pattern=test_runtime.py ;;
        options) pattern=test_options.py ;;
        journal) pattern=test_journal.py ;;
        ui) pattern=test_ui_lifecycle.py ;;
        *) echo "Unknown or unimplemented suite: $2" >&2; exit 2 ;;
    esac
    python3 -m unittest discover -s tests -p "$pattern" -v
else
    echo 'Usage: bash scripts/test.sh [--suite identity|deps|collector|binary|profile|runtime|options|journal|ui]' >&2
    exit 2
fi
