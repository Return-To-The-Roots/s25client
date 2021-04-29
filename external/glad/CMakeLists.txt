# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Note: Sources and headers are generated using http://glad.dav1d.de/ on 07.07.2018
# See glad.h for details

include(ConstrainedOption)
constrained_option(RTTR_OPENGL
    DEFAULT "OGL2.0Compat"
    DESCRIPTION "The OpenGL version to use"
    VALUES OGL2.0Compat OGL2.0 OGL2.1Compat OGL2.1 OGL3.3 GLES2.0)

string(REGEX MATCH "^(OGL|GLES)([0-9])\\.([0-9])(Compat)?$" ogl_match ${RTTR_OPENGL})
if(NOT ogl_match)
    message(FATAL_ERROR "Invalid value for RTTR_OPENGL: ${RTTR_OPENGL}.")
endif()
set(RTTR_OGL_MAJOR ${CMAKE_MATCH_2})
set(RTTR_OGL_MINOR ${CMAKE_MATCH_3})
if(CMAKE_MATCH_0 STREQUAL "GLES")
    set(RTTR_OGL_ES 1)
    set(rttr_glad_folder "OpenGLES")
else()
    set(RTTR_OGL_ES 0)
    set(rttr_glad_folder "OpenGL")
endif()
if(CMAKE_MATCH_4 STREQUAL "Compat")
    set(RTTR_OGL_COMPAT 1)
else()
    set(RTTR_OGL_COMPAT 1)
endif()
get_filename_component(rttr_glad_folder "${rttr_glad_folder}${RTTR_OGL_MAJOR}.${RTTR_OGL_MINOR}${CMAKE_MATCH_4}" ABSOLUTE)
configure_file(openglCfg.hpp.cmake include/openglCfg.hpp @ONLY)

add_library(glad STATIC ${rttr_glad_folder}/src/glad.c)
set(includeFolder ${CMAKE_CURRENT_BINARY_DIR}/include)
file(REMOVE ${includeFolder})
file(COPY ${rttr_glad_folder}/include DESTINATION .)
target_include_directories(glad PUBLIC ${includeFolder})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()
