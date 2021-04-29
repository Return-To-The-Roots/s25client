#!/bin/bash

# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

external/libutil/tools/ci/collectCoverageData.sh \
    '*/build/*' \
    '*/boost/*' \
    '*/external/*' \
    '*/tests/legacyFiles/*' \
    '*/tests/mockupDrivers/*' \
    '*/tests/testHelpers/*' \
