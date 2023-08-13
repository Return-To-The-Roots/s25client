# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(RTTR_Assert_Enabled 2 CACHE STRING "Status of RTTR assertions: 0=Disabled, 1=Enabled, 2=Default(Enabled only in debug)")

file(GLOB COMMON_SRC src/*.cpp)
file(GLOB COMMON_HEADERS include/*.h include/*.hpp)
file(GLOB COMMON_HELPERS_SRC src/helpers/*.cpp)
file(GLOB COMMON_HELPERS_HEADERS include/helpers/*.h include/helpers/*.hpp)
set(ALL_SRC ${COMMON_SRC} ${COMMON_HEADERS} ${COMMON_HELPERS_SRC} ${COMMON_HELPERS_HEADERS})

source_group(src FILES ${COMMON_SRC} ${COMMON_HEADERS})
source_group(helpers FILES ${COMMON_HELPERS_SRC} ${COMMON_HELPERS_HEADERS})

add_library(s25Common STATIC ${ALL_SRC})
target_include_directories(s25Common PUBLIC include)
target_link_libraries(s25Common PUBLIC s25util::common s25util::log Boost::boost)
set_target_properties(s25Common PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_EXTENSIONS OFF)
target_compile_features(s25Common PUBLIC cxx_std_17)

if(RTTR_Assert_Enabled EQUAL 0)
    target_compile_definitions(s25Common PUBLIC RTTR_ENABLE_ASSERTS=0)
elseif(RTTR_Assert_Enabled EQUAL 1)
    target_compile_definitions(s25Common PUBLIC RTTR_ENABLE_ASSERTS=1)
endif()
if(HAVE_STRUCT_TIMESPEC)
    target_compile_definitions(s25Common PUBLIC HAVE_STRUCT_TIMESPEC)
endif()

if(WIN32)
    target_compile_definitions(s25Common PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

include(EnableWarnings)
enable_warnings(s25Common)
