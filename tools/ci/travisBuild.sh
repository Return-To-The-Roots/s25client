#!/bin/bash

set -euo pipefail

# Information
cmake --version
$CXX --version

INSTALL_DIR="${TRAVIS_BUILD_DIR}/installed"
rm -rf "${INSTALL_DIR}"
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DRTTR_ENABLE_WERROR=ON -DRTTR_EDITOR_ADMINMODE=ON \
    -G "Unix Makefiles" ${ADDITIONAL_CMAKE_FLAGS}
# Travis uses 2 cores
if [[ "${BUILD_TYPE}" == "Release" ]]; then
    make -j2 install
else
    make -j2
fi

# Set runtime path for boost libraries
boostLibDir=`cmake -LA -N . | grep Boost_LIBRARY_DIR_DEBUG | cut -d "=" -f2`
export DYLD_LIBRARY_PATH="${boostLibDir}:${DYLD_LIBRARY_PATH:-}"
export LD_LIBRARY_PATH="${boostLibDir}:${LD_LIBRARY_PATH:-}"

# Execute tests
export RTTR_DISABLE_ASSERT_BREAKPOINT=1
if ! ctest --output-on-failure=1 -j2; then
    cat CMakeCache.txt
    echo "LD:${LD_LIBRARY_PATH:-}"
    echo "DYLD:${DYLD_LIBRARY_PATH:-}"
    ctest --output-on-failure=1 --rerun-failed --extra-verbose
fi
