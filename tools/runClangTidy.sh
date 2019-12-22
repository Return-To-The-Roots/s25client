#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")/.."

if [ ! -f "build/compile_commands.json" ]; then
    echo "Expected a compile_commands.json in build folder.
Check that it is generated (-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)" >&2
    exit 1
fi


NAMES=(run-clang-tidy-8.py run-clang-tidy.py)
for fn in "${NAMES[@]}"; do
    if which "${fn}" &> /dev/null; then
        CLANG_TIDY_CMD="${fn}"
        break
    fi
done

if [ "${CLANG_TIDY_CMD:-}" == "" ] || ! which "${CLANG_TIDY_CMD}" &> /dev/null; then
    echo "clang-tidy not found. Tried: ${NAMES[*]}" >&2
    exit 1
fi

FILTER="$(pwd)/(extras|libs|tests|\
external/(libendian|liblobby|libsiedler2|\
libutil/(tests|lib)|\
mygettext|s-c/src|s25edit|s25update))"

${CLANG_TIDY_CMD} -p build \
    -j $(nproc) \
    -header-filter "${FILTER}" \
    -quiet \
    "$@" \
    "${FILTER}"
