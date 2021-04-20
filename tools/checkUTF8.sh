#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

# Try to convert all source files to UTF-8 to detect encoding errors
# Execute in the project folder (s25client)
find . -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp' -o -name '*.h' \) -exec iconv -f UTF-8 > /dev/null {} \;
