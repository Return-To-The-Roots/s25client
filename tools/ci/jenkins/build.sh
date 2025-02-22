#!/bin/bash

# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Run build for one architecture specified by parameter marker
#
# Parameter marker (replaced by Jenkins): 
# - %architecture% : i.e "windows.i686"
# - %deploy_to% : i.e "nightly" "stable"
#
# Directory structure:
# /.ccache            : CCACHE directory
# /source (read-only) : source directory
# /result             : directory for result files
#

set +x -euo pipefail

if [ -z "$JENKINS_SERVER_COOKIE" ] ; then
    echo "error: this script has to be run by Jenkins only." >&2
    exit 1
fi

###############################################################################

export CCACHE_DIR=/.ccache

deploy_to="%deploy_to%"
architecture="%architecture%"

src_dir=/source
result_dir=/result

# Required by the tests
export USER="${USER:-TestUser}'"

TOOLCHAIN_FILE=
if [ "$(uname -s | tr "[:upper:]" "[:lower:]").$(uname -m)" != "$architecture" ] ; then
    TOOLCHAIN_FILE=$src_dir/cmake/toolchains/c.$architecture.cmake
fi

BUILD_TYPE=Release

rm -rf _CPack_Packages *.tar.bz2 *.zip CMakeFiles CMakeCache.txt

RTTR_VERSION=OFF
if [ "$deploy_to" == "stable" ] ; then
    GIT_TAG=$(git -C $src_dir describe --all --exact-match 2>/dev/null || true)
    if [ -z "$GIT_TAG" ] || [[ ! "$GIT_TAG" =~ tags/v[0-9]+\.[0-9]+\.[0-9]+ ]] ; then
        echo "Tried to publish to stable, but no Git TAG 'vX.Y.Z' was found: $(git -C $src_dir describe --all --exact-match 2>&1)" >&2
        exit 1
    fi
    RTTR_VERSION=${GIT_TAG#"tags/v"}
    echo "Using RTTR_VERSION from GIT_TAG: $RTTR_VERSION" >&2
fi

cmake \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
    -DRTTR_ENABLE_WERROR=ON \
    -DRTTR_USE_STATIC_BOOST=ON \
    -DRTTR_VERSION=$RTTR_VERSION \
    -DRTTR_REVISION=OFF \
    -DRTTR_BUNDLE=ON \
    -DRTTR_EXTERNAL_BUILD_TESTING=ON \
    -DRTTR_INCLUDE_DEVTOOLS=ON \
    $src_dir

make -j4 package

if [ "$(uname -s | tr "[:upper:]" "[:lower:]").$(uname -m)" == "$architecture" ] ; then
    export RTTR_DISABLE_ASSERT_BREAKPOINT=1
    export UBSAN_OPTIONS=print_stacktrace=1
    export AUDIODEV=null                    # Avoid errors like: ALSA lib confmisc.c:768:(parse_card) cannot find card '0'
    export SDL_VIDEODRIVER=dummy            # Avoid crash on travis

    if ! ctest --output-on-failure -j2; then
        echo "error: tests failed. re-running failed tests with extra verbosity now" >&2
        ctest --output-on-failure --rerun-failed --extra-verbose
    fi
fi

files=$(find . -maxdepth 1 -name '*.zip' -o -name '*.tar.bz2')
if [ -z "$files" ] ; then
    echo "error: no artifacts were generated." >&2
    exit 1
fi

cp -av $files $result_dir/
