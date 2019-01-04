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

# Execute tests
export BOOST_TEST_CATCH_SYSTEM_ERRORS="no"
export RTTR_DISABLE_ASSERT_BREAKPOINT=1
ctest --output-on-failure=1 -j2
