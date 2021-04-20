// Copyright (C) 2018 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "makeException.h"
#include <rttr/test/testHelpers.hpp>
#include <boost/system/config.hpp>
#include <boost/test/unit_test.hpp>
#ifdef BOOST_WINDOWS_API
#    include <windows.h>
#    define RTTR_SET_LAST_ERROR(errNum) SetLastError(errNum)
#elif defined(BOOST_POSIX_API)
#    include <cerrno>
#    define RTTR_SET_LAST_ERROR(errNum) errno = errNum
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
