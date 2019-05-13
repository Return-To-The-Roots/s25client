#!/bin/bash

set -euo pipefail

external/libutil/tools/ci/uploadCoverageData.sh \
    '*/external/*'
