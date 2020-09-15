#!/bin/bash

set -euo pipefail

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
    -DRTTR_EDITOR_ADMINMODE=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ${ADDITIONAL_CMAKE_FLAGS}

SRC_DIR="$(cd .. && pwd)"
FILTER="extras|libs|tests|external/(libendian|liblobby|libsiedler2|libutil|mygettext|s25edit|s25update)"

script -q -c "run-clang-tidy-10 -p . \
    -quiet \
    -header-filter \"${SRC_DIR}/(${FILTER})\" \
    \"${SRC_DIR}/(${FILTER})\"" /dev/null \
     2>&1 | tee tidy-output.log | grep -v ' warnings generated.'

ERRORS="$( (grep -E 'warning: |error: ' tidy-output.log || [ $? -eq 1 ]) | sort -u)"
if [[ -n ${ERRORS} ]]; then
    echo "Clang-Tidy found issues. Fix those first"
    echo "${ERRORS}"
    exit 1
fi
