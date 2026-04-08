# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(BZip2 1.0.6 REQUIRED)
gather_dll(BZIP2)
find_package(Protobuf REQUIRED)

set(SOURCES_SUBDIRS )
macro(AddDirectory dir)
    file(GLOB SUB_FILES ${dir}/*.cpp ${dir}/*.h ${dir}/*.hpp ${dir}/*.tpp)
    set(SOURCES_SUBDIRS ${SOURCES_SUBDIRS} ${SUB_FILES})
    source_group(${dir} FILES ${SUB_FILES})
endmacro()

AddDirectory(addons)
AddDirectory(ai)
AddDirectory(ai/aijh/runtime)
AddDirectory(ai/aijh/planning)
AddDirectory(ai/aijh/combat)
AddDirectory(ai/aijh/config)
AddDirectory(ai/aijh/debug)
AddDirectory(dataextractor)
AddDirectory(animation)
AddDirectory(buildings)
AddDirectory(controls)
AddDirectory(desktops)
AddDirectory(drivers)
AddDirectory(factories)
AddDirectory(figures)
AddDirectory(gameData)
AddDirectory(gameTypes)
AddDirectory(helpers)
AddDirectory(ingameWindows)
AddDirectory(lua)
AddDirectory(mapGenerator)
AddDirectory(network)
AddDirectory(nodeObjs)
AddDirectory(notifications)
AddDirectory(ogl)
AddDirectory(pathfinding)
AddDirectory(postSystem)
AddDirectory(random)
AddDirectory(resources)
AddDirectory(world)
file(GLOB SOURCES_OTHER *.cpp *.h)
source_group(other FILES ${SOURCES_OTHER})

set(s25Main_SRCS
    ${SOURCES_OTHER}
    ${SOURCES_SUBDIRS}
        BuildingEventLogger.cpp
        CombatEventLogger.cpp
        CombatLossTracker.cpp
        CountryEventLogger.cpp
        CountryPlotEventLogger.cpp
        MilitaryEventLogger.cpp
        MilitaryStatsHolder.cpp
        TroopsLimitEventLogger.cpp
        ToolPriorityEventLogger.cpp
        WareEventLogger.cpp
        ai/aijh/runtime/AIAdjuster.cpp
        ai/aijh/runtime/AIMapState.cpp
        ai/aijh/runtime/AIWorldQueries.cpp
        ai/aijh/combat/TargetSelector.cpp
        ai/aijh/combat/TargetSelectorAttrition.cpp
        ai/aijh/combat/TargetSelectorBiting.cpp
        ai/aijh/combat/TargetSelectorPrudent.cpp
        ai/aijh/combat/TargetSelectorRandom.cpp
        ai/aijh/planning/BuildingCalculator.cpp
        ai/aijh/config/AIConfig.cpp
        ai/aijh/config/AIConfig.h
        ai/aijh/planning/PlannerHelper.cpp
        ai/aijh/planning/PlannerHelper.h
        ai/aijh/planning/GlobalPositionFinder.cpp
        ai/aijh/planning/GlobalPositionFinder.h
        ai/RatedPoint.h
        ai/aijh/config/WeightParser.h
        ai/aijh/config/WeightParams.h
)

set(WARES_LOG_PROTO ${PROJECT_SOURCE_DIR}/external/proto-repo/wares_log.proto)
protobuf_generate_cpp(WARES_LOG_PROTO_SRCS WARES_LOG_PROTO_HDRS ${WARES_LOG_PROTO})
set_source_files_properties(${WARES_LOG_PROTO_SRCS} PROPERTIES COMPILE_OPTIONS "-Wno-missing-declarations")
list(APPEND s25Main_SRCS ${WARES_LOG_PROTO_SRCS} ${WARES_LOG_PROTO_HDRS})

set(COMBAT_LOG_PROTOS
    ${PROJECT_SOURCE_DIR}/external/proto-repo/commons.proto
    ${PROJECT_SOURCE_DIR}/external/proto-repo/combat_log.proto
)
protobuf_generate_cpp(COMBAT_LOG_PROTO_SRCS COMBAT_LOG_PROTO_HDRS ${COMBAT_LOG_PROTOS})
set_source_files_properties(${COMBAT_LOG_PROTO_SRCS} PROPERTIES COMPILE_OPTIONS "-Wno-missing-declarations")
list(APPEND s25Main_SRCS ${COMBAT_LOG_PROTO_SRCS} ${COMBAT_LOG_PROTO_HDRS})

set(COUNTRY_PLOTS_LOG_PROTO ${PROJECT_SOURCE_DIR}/external/proto-repo/country_plots_log.proto)
protobuf_generate_cpp(COUNTRY_PLOTS_LOG_PROTO_SRCS COUNTRY_PLOTS_LOG_PROTO_HDRS ${COUNTRY_PLOTS_LOG_PROTO})
set_source_files_properties(${COUNTRY_PLOTS_LOG_PROTO_SRCS} PROPERTIES COMPILE_OPTIONS "-Wno-missing-declarations")
list(APPEND s25Main_SRCS ${COUNTRY_PLOTS_LOG_PROTO_SRCS} ${COUNTRY_PLOTS_LOG_PROTO_HDRS})

add_library(s25Main STATIC ${s25Main_SRCS})

set(AIJH_DEPENDENCY_GUARD_SCRIPT ${PROJECT_SOURCE_DIR}/cmake/CheckAIJHDependencies.cmake)
add_custom_target(ai_jh_dependency_guards
    COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${AIJH_DEPENDENCY_GUARD_SCRIPT}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking AI JH dependency boundaries"
)
add_dependencies(s25Main ai_jh_dependency_guards)

if(BUILD_TESTING)
    add_test(NAME ai_jh_dependency_guards
             COMMAND ${CMAKE_COMMAND} -DROOT=${PROJECT_SOURCE_DIR} -P ${AIJH_DEPENDENCY_GUARD_SCRIPT}
             WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

target_include_directories(s25Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(s25Main PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(s25Main PUBLIC cxx_std_17)
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(s25Main PUBLIC
    siedler2
    lobby_c
    s25util::common
    s25util::network
    mygettext
    s25Common
    rttrConfig
    gamedata
    glad
    driver
    yaml-cpp
    nlohmann_json::nlohmann_json
    Boost::filesystem Boost::disable_autolinking
    PRIVATE BZip2::BZip2 Boost::iostreams Boost::locale Boost::nowide samplerate_cpp yaml-cpp
            protobuf::libprotobuf
)

if(WIN32)
    include(CheckIncludeFiles)
    check_include_files("windows.h;dbghelp.h" HAVE_DBGHELP_H)
    if(HAVE_DBGHELP_H)
        target_compile_definitions(s25Main PUBLIC HAVE_DBGHELP_H)
    endif()
endif()

# For clock_gettime etc. this is required on some platforms/compilers
find_library(LIBRT rt)
if(LIBRT)
    target_link_libraries(s25Main PUBLIC ${LIBRT})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(s25Main PUBLIC ${CMAKE_DL_LIBS}) # For dynamic driver loading (DriverWrapper)
endif()

include(EnableWarnings)
enable_warnings(s25Main)
