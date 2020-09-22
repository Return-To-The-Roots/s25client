#!/bin/bash

set -euo pipefail

WHAT="${1}"

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
    -DRTTR_EDITOR_ADMINMODE=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ${ADDITIONAL_CMAKE_FLAGS}

SRC_DIR="$(cd .. && pwd)"
MAIN_FILTER="extras|libs|tests"
EXTERNAL_FILTER="external/(libendian|liblobby|libsiedler2|mygettext|s25edit|s25update)|\
external/libutil/(tests|lib)"
HEADER_FILTER="${MAIN_FILTER}|${EXTERNAL_FILTER}"

if [[ "${WHAT}" == "main" ]]; then
    SRC_FILTER="${MAIN_FILTER}"
elif [[ "${WHAT}" == "external" ]]; then
    SRC_FILTER="${EXTERNAL_FILTER}"
else
    echo "Invalid param: ${WHAT}"
    exit 1
fi

script -q -c "run-clang-tidy-10 -p . \
    -j2 \
    -quiet \
    -header-filter \"${SRC_DIR}/(${HEADER_FILTER})\" \
    \"${SRC_DIR}/(${SRC_FILTER})\"" /dev/null \
     2>&1 | tee tidy-output.log | grep -v ' warnings generated.'

ERRORS="$( (grep -E 'warning: |error: ' tidy-output.log || [ $? -eq 1 ]) | sort -u)"
if [[ -n ${ERRORS} ]]; then
    echo "Clang-Tidy found issues. Fix those first"
    echo "${ERRORS}"
    exit 1
fi
