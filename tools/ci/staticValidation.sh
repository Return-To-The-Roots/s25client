#!/bin/bash

# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

SRC_DIR="${1:?SRC_DIR missing}"

cd "${SRC_DIR}"

nonUTF8Files="$(tools/checkUTF8.sh)"
if [[ "${nonUTF8Files}" != "" ]]; then
  echo "Files not in UTF8 encoding: ${nonUTF8Files}"
  exit 1
fi

canonicalBugs="$(grep -r -l --include \*.h --include \*.cpp --include \*.hpp --include \*.tpp "::canonical(" || true)"
if [[ "${canonicalBugs}" != "" ]]; then
  echo "Following files use buggy boost::filesystem::canonical: ${canonicalBugs}"
  echo "If this is only used on non-windows use canonical unqualified: using bfs::canonical; canonical(...)"
  exit 1
fi
