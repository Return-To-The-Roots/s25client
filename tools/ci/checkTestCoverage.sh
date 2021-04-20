#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

export PATH="/tmp/lcov/bin:$PATH"

lcov --extract coverage.info '*/tests/*' -o testcov.info > /dev/null
lcov --remove coverage.info '*/tests/*' -o srccov.info > /dev/null

"$(dirname "$0")/checkTestCoverage.py" testcov.info
