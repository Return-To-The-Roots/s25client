#!/bin/bash

set -euo pipefail

external/libutil/tools/ci/collectCoverageData.sh \
    '*/build/*' \
    '*/boost/*' \
    '*/external/*' \
    '*/tests/legacyFiles/*' \
    '*/tests/mockupDrivers/*' \
    '*/tests/testHelpers/*' \
