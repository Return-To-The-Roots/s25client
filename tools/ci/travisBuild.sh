#!/bin/bash

set -euo pipefail

# Information
cmake --version
$CXX --version

if [[ "${BUILD_TYPE}" == "Release" ]]; then
    RTTR_BUNDLE="ON"
    MAKE_TARGET="install"
else
    RTTR_BUNDLE="OFF"
    MAKE_TARGET="all"
fi

INSTALL_DIR="${TRAVIS_BUILD_DIR}/installed"
rm -rf "${INSTALL_DIR}"
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DRTTR_ENABLE_WERROR=ON \
    -DRTTR_EDITOR_ADMINMODE=ON \
    -DRTTR_BUNDLE="${RTTR_BUNDLE}" \
    -G "Unix Makefiles" ${ADDITIONAL_CMAKE_FLAGS}

# Travis uses 2 cores
make -j2 ${MAKE_TARGET}

# Set runtime path for boost libraries
boostLibDir=`cmake -LA -N . | grep Boost_LIBRARY_DIR_DEBUG | cut -d "=" -f2`
export DYLD_LIBRARY_PATH="${boostLibDir}:${DYLD_LIBRARY_PATH:-}"
export LD_LIBRARY_PATH="${boostLibDir}:${LD_LIBRARY_PATH:-}"

# Execute tests
export RTTR_DISABLE_ASSERT_BREAKPOINT=1
if ! ctest --output-on-failure -j2; then
    cat CMakeCache.txt
    echo "LD:${LD_LIBRARY_PATH:-}"
    echo "DYLD:${DYLD_LIBRARY_PATH:-}"
    ctest --output-on-failure --rerun-failed --extra-verbose
fi
