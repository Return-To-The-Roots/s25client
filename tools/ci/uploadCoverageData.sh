#!/bin/bash

set -euo pipefail

external/libutil/tools/ci/uploadCoverageData.sh \
    '*/external/full-contrib-msvc/*' \
    '*/external/glad/*' \
    '*/external/kaguya/*' \
    '*/external/languages/*' \
    '*/external/libendian/*' \
    '*/external/liblobby/*' \
    '*/external/lua/*' \
    '*/external/macos/*' \
    '*/external/s25edit/*' \
    '*/external/s25update/*' \
    '*/external/s-c/*' \
    '*/external/turtle/*' \
    '*/tests/*'
