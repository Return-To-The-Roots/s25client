# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Add Icon to Application
if(WIN32)
    set(rcDir ${PROJECT_SOURCE_DIR}/data/win32)
    if(CMAKE_HOST_WIN32)
        set(s25client_RC ${rcDir}/s25client.rc)
    else()
        add_definitions(-DAFX_TARG_DEU)
        set(s25client_RC ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o)
        add_custom_command(OUTPUT ${s25client_RC}
                           COMMAND ${CMAKE_RC_COMPILER} -I${rcDir}/ -i${rcDir}/s25client.rc -o ${s25client_RC})

    endif()
else()
    set(s25client_RC )
endif()

add_executable(s25client s25client.cpp commands.cpp ${s25client_RC})
target_link_libraries(s25client PRIVATE s25Main Boost::program_options Boost::nowide rttr::vld)
add_dependencies(s25client drivers)

if(WIN32)
    target_include_directories(s25client PRIVATE ${rcDir})
    target_link_libraries(s25client PRIVATE ole32 ws2_32 shlwapi imagehlp)
    if(MSVC)
        set_target_properties(s25client PROPERTIES LINK_FLAGS /MANIFEST:NO)
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX)
        set_target_properties(s25client PROPERTIES LINK_FLAGS -Wl,--stack,8388608)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(s25client PRIVATE pthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_libraries(s25client PRIVATE execinfo)
ENDif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT s25client)
endif()

if(WIN32)
    include(GatherDll)
    gather_dll_copy(s25client)
endif()

INSTALL(TARGETS s25client RUNTIME DESTINATION ${RTTR_BINDIR})
