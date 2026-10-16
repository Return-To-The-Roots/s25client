#!/bin/bash

# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Print the versions of the build environment and write the full list of
# installed packages (a simple SBOM of the build image) to the result directory.
#
# Parameter marker (replaced by Jenkins):
# - %architecture% : i.e "windows.i686"
#
# Directory structure:
# /result : directory for result files
#

set +x -euo pipefail

architecture="%architecture%"

sbom_dir=/result/sbom
sbom_file=$sbom_dir/$architecture.txt

mkdir -p "$sbom_dir"

echo "Architecture: $architecture"
echo "OS: $(. /etc/os-release && echo "$PRETTY_NAME") (Debian $(cat /etc/debian_version))"

for tool in gcc g++ clang cmake ccache ninja make \
            x86_64-w64-mingw32-g++ i686-w64-mingw32-g++ o64-clang++ oa64-clang++ ; do
    if command -v "$tool" >/dev/null 2>&1 ; then
        printf '%s: %s\n' "$tool" "$("$tool" --version 2>&1 | head -n1)"
    fi
done

if [ -d /usr/lib/apple/SDKs ] ; then
    ls /usr/lib/apple/SDKs | grep '\.sdk$' | sed 's/^/sdk: /' || true
fi

# Full package list, one "name version" per line
dpkg-query -W -f='${binary:Package} ${Version}\n' | sort > "$sbom_file"

echo "Packages (full list in artifact sbom/$architecture.txt):"
grep -E '^(libboost|apple-|mingw-|linux-|libsdl|libcurl|libminiupnpc|libbz2|libogg|libvorbis|libiconv|cmake|ccache|clang|gcc|g\+\+)' "$sbom_file" | sed 's/^/  /' || true
