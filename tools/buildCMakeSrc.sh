#!/bin/sh

# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

files=$(find "$1" -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp' -o -name '*.h' \))
for file in $files; do
    echo $file
done