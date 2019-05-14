#!/bin/bash

set -euo pipefail

external/libutil/tools/ci/uploadCoverageData.sh \
    '*/external/*' \
    '*/tests/legacyFiles/*' \
    '*/tests/mockupDrivers/*' \
    '*/tests/testHelpers/*' \
