FIND_PACKAGE(BZip2 REQUIRED)
gather_dll(BZIP2)
FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS filesystem iostreams system program_options locale chrono)

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
AddDirectory(world)
FILE(GLOB SOURCES_OTHER *.cpp *.h)
SOURCE_GROUP(other FILES ${SOURCES_OTHER})

SET(s25Main_SRCS
	${SOURCES_OTHER}
	${SOURCES_SUBDIRS}
)

ADD_LIBRARY(s25Main STATIC ${s25Main_SRCS})
target_include_directories(s25Main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(s25Main SYSTEM PRIVATE ${BZIP2_INCLUDE_DIR} PUBLIC ${Boost_INCLUDE_DIR})

# Disable automatic boost linking
target_compile_definitions(s25Main PUBLIC BOOST_ALL_NO_LIB)
target_link_libraries(s25Main PUBLIC
	siedler2
	lobby_c
	s25util
	mygettext
	s25Common
	rttrConfig
	gamedata
  glad
  driver
	${BZIP2_LIBRARIES}
	${Boost_LIBRARIES}
	PRIVATE utf8
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
