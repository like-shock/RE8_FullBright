#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname -- "$0")/.."
compiler=${FB_WINDOWS_CC:-x86_64-w64-mingw32-gcc-posix}
command -v "$compiler" >/dev/null || { echo "Missing Windows compiler: $compiler" >&2; exit 2; }
python3 tools/fetch_deps.py
mkdir -p build/lua
lua_objects=()
for source in build/deps/lua-5.4.3/src/*.c; do
    case "${source##*/}" in lua.c|luac.c) continue ;; esac
    object="build/lua/${source##*/}.o"
    "$compiler" -std=c11 -O2 -DLUA_USE_WINDOWS -c "$source" -o "$object"
    lua_objects+=("$object")
done
"$compiler" -std=c11 -O2 -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off \
    -Isrc -Ibuild/deps -Ibuild/deps/lua-5.4.3/src -shared -static-libgcc -Wl,--no-insert-timestamp \
    -Wl,--dynamicbase -Wl,--nxcompat src/plugin.c src/build_identity.c src/sha256.c src/profile.c src/runtime.c \
    src/options.c src/options_ui.c src/journal.c src/file_io.c "${lua_objects[@]}" -luser32 \
    -o build/RE8_FullBright.dll
"$compiler" --version > build/compiler.txt
python3 tools/build_manifest.py
