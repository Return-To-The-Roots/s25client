// Copyright (c) 2018 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "makeException.h"
#include <rttr/test/testHelpers.hpp>
#include <boost/system/config.hpp>
#include <boost/test/unit_test.hpp>
#ifdef BOOST_WINDOWS_API
#include <windows.h>
#define RTTR_SET_LAST_ERROR(errNum) SetLastError(errNum)
#elif defined(BOOST_POSIX_API)
#include <cerrno>
#define RTTR_SET_LAST_ERROR(errNum) errno = errNum
#endif

BOOST_AUTO_TEST_SUITE(Helpers)

BOOST_AUTO_TEST_CASE(MakeExceptionDefault)
{
    RTTR_CHECK_EXCEPTION_MSG(throw makeException("Test"), std::runtime_error, "Test");
    RTTR_CHECK_EXCEPTION_MSG(throw makeException("Test", 1, "bar", 42), std::runtime_error, "Test1bar42");
}

struct CustomException : std::runtime_error
{
    CustomException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

BOOST_AUTO_TEST_CASE(MakeExceptionCustom)
{
    RTTR_CHECK_EXCEPTION_MSG(throw makeException<CustomException>("Test"), CustomException, "Test");
    RTTR_CHECK_EXCEPTION_MSG(throw makeException<CustomException>("Test", 1, "bar", 42), CustomException, "Test1bar42");
}

BOOST_AUTO_TEST_CASE(MakeLastSystemError)
{
    RTTR_SET_LAST_ERROR(0);
    BOOST_TEST(makeLastSystemError().code() == std::error_code(0, std::system_category()));

    RTTR_SET_LAST_ERROR(42);
    BOOST_TEST(makeLastSystemError().code() == std::error_code(42, std::system_category()));

    RTTR_SET_LAST_ERROR(99);
    const auto error = makeLastSystemError("Test", " error", " msg");
    BOOST_TEST(error.code() == std::error_code(99, std::system_category()));
    BOOST_TEST(std::string(error.what()).find("Test error msg:") == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
