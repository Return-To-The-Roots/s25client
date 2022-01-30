// Copyright (C) 2005 - 2022 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>

namespace rttr
{
namespace test
{
  /// Base directory with the sources
  const boost::filesystem::path rttrBaseDir = "@RTTR_BASE_DIR@";
  /// Directory in the build folder usable for test data
  const boost::filesystem::path rttrTestDataDirOut = "@RTTR_TESTDATA_DIR_OUT@";
  /// Path to libsiedler2 test files
  const boost::filesystem::path libsiedler2TestFilesDir = "@RTTR_LIBSIEDLER2_TEST_FILES_DIR@";
}
}
