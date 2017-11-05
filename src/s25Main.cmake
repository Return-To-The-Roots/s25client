if(WIN32)
    include(CheckIncludeFiles)
    check_include_files("windows.h;dbghelp.h" HAVE_DBGHELP_H)
    if(HAVE_DBGHELP_H)
        add_definitions(-DHAVE_DBGHELP_H)
    endif()
endif()


FILE(GLOB SOURCES_OTHER *.cpp *.h)
SOURCE_GROUP(other FILES ${SOURCES_OTHER})

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
AddDirectory(nodeObjs)
AddDirectory(notifications)
AddDirectory(ogl)
AddDirectory(pathfinding)
AddDirectory(postSystem)
AddDirectory(random)
AddDirectory(world)

INCLUDE(AddFileDependencies)
ADD_FILE_DEPENDENCIES(${CMAKE_CURRENT_SOURCE_DIR}/RTTR_Version.cpp ${CMAKE_BINARY_DIR}/build_version_defines.h)

SET(s25Main_SRCS
	${SOURCES_OTHER}
	${SOURCES_SUBDIRS}
)

include_directories(${UTF8_INCLUDE_DIR})
ADD_LIBRARY(s25Main STATIC ${s25Main_SRCS})
ADD_DEPENDENCIES(s25Main updateversion)
target_include_directories(s25Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(s25Main
	PUBLIC siedler2
	PUBLIC lobby_c
	PUBLIC s25util
	PUBLIC mygettext
	PUBLIC s25Common
	PUBLIC rttrConfig
	PUBLIC ${BZIP2_LIBRARIES}
	PUBLIC ${OPENGL_gl_LIBRARY}
	PUBLIC ${LUA_LIBRARY}
	PUBLIC ${Boost_LIBRARIES}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	TARGET_LINK_LIBRARIES(s25Main PUBLIC dl)
ENDif()
