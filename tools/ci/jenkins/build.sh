#!/bin/bash
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

TOOLCHAIN_FILE=
if [ "$(uname -s | tr "[:upper:]" "[:lower:]").$(uname -m)" != "$architecture" ] ; then
    TOOLCHAIN_FILE=$src_dir/cmake/toolchains/c.$architecture.cmake
fi

BUILD_TYPE=Release

rm -rf _CPack_Packages *.tar.bz2 *.zip CMakeFiles CMakeCache.txt

RTTR_VERSION=OFF
if [ "$deploy_to" == "stable" ] ; then
    GIT_TAG=$(git describe --exact-match 2>/dev/null || true)
    if [ -z "$GIT_TAG" ] || [[ ! "$GIT_TAG" =~ v[0-9]+\.[0-9]+\.[0-9]+ ]] ; then
        echo "Tried to publish to stable, but no Git TAG 'vX.Y.Z' was found" >&2
        exit 1
    fi
    RTTR_VERSION=${GIT_TAG#"v"}
fi

cmake \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
    -DRTTR_ENABLE_WERROR=ON \
    -DRTTR_USE_STATIC_BOOST=ON \
    -DRTTR_VERSION=$RTTR_VERSION \
    -DRTTR_REVISION=OFF \
    -DRTTR_BUNDLE=ON \
    $src_dir

make -j4 package

files=$(find . -maxdepth 1 -name '*.zip' -o -name '*.tar.bz2')
if [ -z "$files" ] ; then
    echo "error: no artifacts were generated." >&2
    exit 1
fi

cp -av $files $result_dir/
