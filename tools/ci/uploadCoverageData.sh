#!/bin/bash

set -euo pipefail

external/libutil/tools/ci/uploadCoverageData.sh \
    '*/build/*' \
    '*/external/*' \
    '*/tests/legacyFiles/*' \
    '*/tests/mockupDrivers/*' \
    '*/tests/testHelpers/*' \
