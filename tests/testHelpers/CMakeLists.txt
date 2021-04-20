# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(testHelpers STATIC
    rttr/test/BaseFixture.cpp
    rttr/test/BaseFixture.hpp
    rttr/test/BufferedWriter.hpp
    rttr/test/Fixture.cpp
    rttr/test/Fixture.hpp
    rttr/test/LocaleResetter.hpp
    rttr/test/LogAccessor.hpp
    rttr/test/MockClock.hpp
    rttr/test/random.cpp
    rttr/test/random.hpp
    rttr/test/stubFunction.hpp
    rttr/test/testHelpers.hpp
    rttr/test/TmpFolder.hpp
)
target_link_libraries(testHelpers PUBLIC s25Common PRIVATE rttrConfig Boost::unit_test_framework)
target_include_directories(testHelpers PUBLIC .)
enable_warnings(testHelpers)

# Heuristically guess if we are compiling against dynamic boost
if(NOT Boost_USE_STATIC_LIBS AND NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY MATCHES "\\${CMAKE_STATIC_LIBRARY_SUFFIX}\$")
    target_compile_definitions(testHelpers PRIVATE BOOST_TEST_DYN_LINK)
endif()
