#!/usr/bin/env bash
# Reads the game directory; creates a new report directory outside it.
set -euo pipefail
umask 077
if [[ $# -ne 2 ]]; then
    echo 'Usage: bash collect_environment.sh GAME_DIR NEW_OUTPUT_DIR' >&2
    exit 2
fi
if [[ ! -f "$1/re8.exe" || ! -r "$1/re8.exe" ]]; then
    echo 'Readable re8.exe not found; no report created.' >&2
    exit 2
fi
game=$(realpath -- "$1")
out=$(realpath -m -- "$2")
case "$out/" in
    "$game/"*) echo 'Report must be outside the game directory.' >&2; exit 3 ;;
esac
if ! mkdir -- "$out"; then
    echo 'Use a new output directory with an existing parent.' >&2
    exit 3
fi
trap 'echo "Collection failed; report directory may be incomplete." >&2' ERR
exe_sha=$(sha256sum < "$game/re8.exe")
exe_sha=${exe_sha%% *}
framework_sha=missing
if [[ -f "$game/dinput8.dll" ]]; then
    framework_sha=$(sha256sum < "$game/dinput8.dll")
    framework_sha=${framework_sha%% *}
fi
# VDF is data: never source or evaluate it as shell code.
manifest="$(dirname -- "$(dirname -- "$game")")/appmanifest_1196590.acf"
steam_build_id=unconfirmed
if [[ -r "$manifest" ]]; then
    value() { sed -n 's/^[[:space:]]*"'"$1"'"[[:space:]]*"\([^"]*\)"[[:space:]]*$/\1/p' "$manifest"; }
    if [[ $(value appid) == 1196590 && $(value installdir) == "$(basename -- "$game")" ]]; then
        candidate=$(value buildid)
        if [[ "$candidate" =~ ^[0-9]+$ ]]; then steam_build_id=$candidate; fi
    fi
fi
{
    printf 'report_schema=1\ncollected_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'exe_sha256=%s\nframework_sha256=%s\nsteam_build_id=%s\n' "$exe_sha" "$framework_sha" "$steam_build_id"
    printf 'proton_version=unconfirmed\nreframework_version=unconfirmed\n'
    printf 'graphics_rt=unconfirmed\nresolution=unconfirmed\nfps_limit=unconfirmed\ntdp=unconfirmed\n'
    if [[ -r /etc/os-release ]]; then
        sed -n -e 's/^PRETTY_NAME=/os_pretty_name=/p' -e 's/^VERSION_ID=/os_version_id=/p' /etc/os-release
    fi
    printf '\n# Record Proton and REFramework versions from their UI; a DLL hash is not a version.\n'
} > "$out/environment.txt"
printf 'Saved: %s/environment.txt\n' "$out"
