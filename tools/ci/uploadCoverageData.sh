#!/bin/bash

set -euo pipefail

# Information
"${GCOV}" --version
lcov --version || true

# capture coverage info
lcov --gcov-tool "${GCOV}" --directory build --capture --output-file coverage.info --rc lcov_branch_coverage=1 > /dev/null
# Remove everything from /usr (unrelated), external folder (3rd party data), test code
lcov --remove coverage.info '/usr/*' '*/external/*' "${HOME}"'/.cache/*' '*/tests/*' --output-file coverage.info > /dev/null
# Debug output
lcov --list coverage.info

# Coverage.io
bash <(curl -s https://codecov.io/bash) -f coverage.info || echo "Codecov did not collect coverage reports"

# Coveralls
coveralls-lcov coverage.info
