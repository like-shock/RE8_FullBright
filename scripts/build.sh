#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "$0")/.."
compiler=${FB_WINDOWS_CC:-x86_64-w64-mingw32-gcc-posix}
command -v "$compiler" >/dev/null || { echo "Missing Windows compiler: $compiler" >&2; exit 2; }
python3 tools/fetch_deps.py
mkdir -p build
"$compiler" -std=c11 -O2 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off \
    -Isrc -Ibuild/deps -shared -static-libgcc -Wl,--no-insert-timestamp \
    -Wl,--dynamicbase -Wl,--nxcompat src/plugin.c src/build_identity.c src/sha256.c \
    -o build/RE8_FullBright.dll
"$compiler" --version > build/compiler.txt
python3 tools/build_manifest.py
