find_package(BZip2 1.0.6 REQUIRED)
gather_dll(BZIP2)

SET(SOURCES_SUBDIRS )
MACRO(AddDirectory dir)
    FILE(GLOB SUB_FILES ${dir}/*.cpp ${dir}/*.h ${dir}/*.hpp ${dir}/*.tpp)
    SET(SOURCES_SUBDIRS ${SOURCES_SUBDIRS} ${SUB_FILES})
    SOURCE_GROUP(${dir} FILES ${SUB_FILES})
ENDMACRO()

AddDirectory(addons)
AddDirectory(ai)
AddDirectory(ai/aijh)
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
FILE(GLOB SOURCES_OTHER *.cpp *.h)
SOURCE_GROUP(other FILES ${SOURCES_OTHER})

SET(s25Main_SRCS
    ${SOURCES_OTHER}
    ${SOURCES_SUBDIRS}
)

ADD_LIBRARY(s25Main STATIC ${s25Main_SRCS})
target_include_directories(s25Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(s25Main PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(s25Main PUBLIC cxx_std_14)

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
    Boost::filesystem Boost::disable_autolinking
    PRIVATE BZip2::BZip2 Boost::iostreams Boost::locale Boost::nowide samplerate_cpp
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
