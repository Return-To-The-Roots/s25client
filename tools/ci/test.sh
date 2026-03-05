#!/bin/bash

# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

# Information
cmake --version
$CXX --version

cd build

# Set runtime path for boost libraries
if ! CMAKE_VARS="$(cmake -LA -N .)"; then
    echo "Failed to get CMake variables"
    exit 1
fi
if echo "${CMAKE_VARS}" | grep -q Boost_LIBRARY_DIR_DEBUG; then
    boostLibDir="$(echo "${CMAKE_VARS}" | grep Boost_LIBRARY_DIR_DEBUG | cut -d "=" -f2)"
else
    boostLibDir="$(echo "${CMAKE_VARS}" | grep Boost_DIR | cut -d "=" -f2)"
    boostLibDir="${boostLibDir%/lib/*}/lib"
fi
# ... but not if it is a system boost because that would fail on OSX due to messing up the search order
if ! [[ "$boostLibDir" =~ ^/usr/ ]]; then
	export DYLD_LIBRARY_PATH="${boostLibDir}${DYLD_LIBRARY_PATH+":$DYLD_LIBRARY_PATH"}"
	export LD_LIBRARY_PATH="${boostLibDir}${LD_LIBRARY_PATH+":$LD_LIBRARY_PATH"}"
fi

# Execute tests
export RTTR_DISABLE_ASSERT_BREAKPOINT=1
export UBSAN_OPTIONS=print_stacktrace=1
export AUDIODEV=null # Avoid errors like: ALSA lib confmisc.c:768:(parse_card) cannot find card '0'
export SDL_VIDEODRIVER=dummy # Avoid crash on travis
if ! ctest --output-on-failure -C "${BUILD_TYPE}" -j2; then
    echo "::group::CMake cache"
    cat CMakeCache.txt
    echo "::endgroup::"
    echo "LD:${LD_LIBRARY_PATH:-}"
    echo "DYLD:${DYLD_LIBRARY_PATH:-}"
    echo "::group::Rerun failed tests only"
    ctest --output-on-failure -C "${BUILD_TYPE}" --rerun-failed --extra-verbose
    echo "::endgroup::"
fi
