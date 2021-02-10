#!/bin/bash

set -euo pipefail

export PATH="/tmp/lcov/bin:$PATH"

lcov --extract coverage.info '*/tests/*' -o testcov.info > /dev/null
lcov --remove coverage.info '*/tests/*' -o srccov.info > /dev/null

"$(dirname "$0")/checkTestCoverage.py" testcov.info
