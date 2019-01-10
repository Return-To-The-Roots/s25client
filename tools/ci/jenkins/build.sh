#!/bin/bash
###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Run build for one architecture specified by parameter marker
#
# Parameter marker (replaced by Jenkins): 
# - %architecture% : i.e "windows.i686"
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
src_dir=/source
result_dir=/result
architecture="%architecture%"

TOOLCHAIN_FILE=
if [ "$(uname -s | tr "[:upper:]" "[:lower:]").$(uname -m)" != "$architecture" ] ; then
    TOOLCHAIN_FILE=$src_dir/cmake/toolchains/c.$architecture.cmake
fi

BUILD_TYPE=Release

rm -rf _CPack_Packages

cmake \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
    -DRTTR_ENABLE_WERROR=ON \
    -DRTTR_USE_STATIC_BOOST=ON \
    -DRTTR_VERSION=OFF \
    -DRTTR_REVISION=OFF \
    $src_dir

make -j4 package

files=$(find . -maxdepth 1 -name '*.zip' -o -name '*.tar.bz2')
if [ -z "$files" ] ; then
    echo "error: no artifacts were generated." >&2
    exit 1
fi

cp -av $files $result_dir/
