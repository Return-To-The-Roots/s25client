#pragma once

#include <boost/filesystem/path.hpp>

/// Base directory with the sources
#cmakedefine RTTR_BASE_DIR "@RTTR_BASE_DIR@"
/// Path to libsiedler2 test files
#cmakedefine RTTR_LIBSIEDLER2_TEST_FILES_DIR "@RTTR_LIBSIEDLER2_TEST_FILES_DIR@"
namespace rttr
{
namespace test
{
  const boost::filesystem::path rttrBaseDir = RTTR_BASE_DIR;
  const boost::filesystem::path libsiedler2TestFilesDir = RTTR_LIBSIEDLER2_TEST_FILES_DIR;
}
}
