#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

# Run IWYU (Include-What-You_Use) on the current build

if [ -d "build" ]; then
    cd build
fi

if [ ! -f compile_commands.json ]; then
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
fi

if [ $? -ne 0 ]; then
    echo "Error during cmake -> exit"
    exit 1
fi

iwyuBinary=`which include-what-you-use`
iwyuTool=`which iwyu_tool.py`

${iwyuTool} -j$(nproc) -p . -- --no_comments --quoted_includes_first --pch_in_code
