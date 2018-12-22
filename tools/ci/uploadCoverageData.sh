#!/bin/bash

set -euo pipefail

# Information
"${GCOV}" --version
lcov --version || true

# Coverage.io
find . -type f -name '*.gcno' -exec "${GCOV}" -lpb {} +
files=$(find . -type f -name '*.gcov' -not -path '*/conftest_*.c.gcov')
if [ "${files}" != "" ]; then
    bash <(curl -s https://codecov.io/bash) -f "${files}"
fi

# Coveralls
lcov --gcov-tool "${GCOV}" --directory build --capture --output-file coverage.info --rc lcov_branch_coverage=1 > /dev/null
# Remove everything from /usr (unrelated), external folder (3rd party data), test code
lcov --gcov-tool "${GCOV}" --remove coverage.info '/usr/*' 'external/*' 'tests/*' --output-file coverage.info > /dev/null
# Debug output
lcov --list coverage.info
coveralls-lcov coverage.info
