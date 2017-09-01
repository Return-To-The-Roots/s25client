FIND_PACKAGE(BZip2 REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Gettext REQUIRED)
FIND_PACKAGE(Boost 1.55.0 COMPONENTS filesystem iostreams system program_options locale REQUIRED)


################################################################################

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(SYSTEM
	${BZIP2_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/contrib/kaguya/include
	)

################################################################################

FILE(GLOB RTTR_PO_FILES ../RTTR/languages/*.po)

GETTEXT_CREATE_TRANSLATIONS(../RTTR/languages/rttr.pot ALL
							${RTTR_PO_FILES}
						   )

################################################################################

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	CORRECT_LIB(OPENGL_gl_LIBRARY OpenGL)
	CORRECT_LIB(SDL_LIBRARY SDL)

	# Add the SDL-include flags to an apple build
	INCLUDE_DIRECTORIES(SYSTEM ${SDL_INCLUDE_DIR})

	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/macos)
	ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS -framework OpenGL)
ENDif()

################################################################################
# LUA
################################################################################

SET(LUA_VERSION "52")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	if("${PLATFORM_ARCH}" STREQUAL "i386" )
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/win32")
	ELSE()
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/win64")
	ENDIF()
ENDif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	if("${PLATFORM_ARCH}" STREQUAL "i386" )
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/lin32")
	ELSE()
		SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/lin64")
	ENDIF()
ENDif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

### XXX Add FindLua52
#find_package(Lua52 REQUIRED)
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
	INCLUDE_DIRECTORIES(SYSTEM "/usr/local/include/lua52")
	SET(LUA_LIB "/usr/local/lib/liblua${LUA_VERSION}.a")
ENDif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	SET(LUA_DIR "${CMAKE_SOURCE_DIR}/contrib/lua/mac")
ENDif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")

IF(NOT DEFINED LUA_LIB)
	if(MSVC)
		IF(MSVC_VERSION GREATER 1800) # MSVC 2015
			SET(LUA_DIR "${RTTR_CONTRIB_DIR}/VS2015/lib")
		ELSE()
			SET(LUA_DIR "${RTTR_CONTRIB_DIR}/lib" )
		ENDIF()
		if("${PLATFORM_ARCH}" STREQUAL "i386")
			SET(LUA_DIR "${LUA_DIR}/x86")
		ELSE()
			SET(LUA_DIR "${LUA_DIR}/x64")
		ENDIF()
		SET(LUA_LIB "${LUA_DIR}/lua${LUA_VERSION}.lib" CACHE FILEPATH "Path to lua lib")
	ELSE ()
		SET(LUA_LIB "${LUA_DIR}/liblua${LUA_VERSION}.a")
	ENDif()
ENDif()

MESSAGE(STATUS "Checking LUA library at ${LUA_LIB}")
if(NOT EXISTS "${LUA_LIB}" )
	SET(LUA_LIB "/usr/lib/liblua5.2.a")
	INCLUDE_DIRECTORIES(SYSTEM "/usr/include/lua5.2")
ENDif(NOT EXISTS "${LUA_LIB}" )

if(NOT EXISTS "${LUA_LIB}" )
	SET(LUA_LIB "/usr/lib/liblua.so")
	INCLUDE_DIRECTORIES(SYSTEM "/usr/include/")
ENDif(NOT EXISTS "${LUA_LIB}" )

if(NOT EXISTS "${LUA_LIB}" )
	SET(LUA_LIB "/usr/local/lib/liblua-5.2.so")
	INCLUDE_DIRECTORIES(SYSTEM "/usr/local/include/")
ENDif(NOT EXISTS "${LUA_LIB}" )

MESSAGE(STATUS "Using LUA library from ${LUA_LIB}")

IF(NOT DEFINED LUA_INCLUDE)
	INCLUDE_DIRECTORIES(SYSTEM "${LUA_DIR}/include")
ELSE()
	INCLUDE_DIRECTORIES(SYSTEM "${LUA_INCLUDE}")
ENDIF()

SET(RTTR_Assert_Enabled 2 CACHE STRING "Status of RTTR assertions: 0=Disabled, 1=Enabled, 2=Default(Enabled only in debug)")
IF("${RTTR_Assert_Enabled}" EQUAL 0)
	ADD_DEFINITIONS(-DRTTR_ENABLE_ASSERTS=0)
ELSEIF("${RTTR_Assert_Enabled}" EQUAL 1)
	ADD_DEFINITIONS(-DRTTR_ENABLE_ASSERTS=1)
ENDIF()

################################################################################

if(MSVC)
	# disable warning 4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
	ADD_DEFINITIONS(/wd4267)

 	SET(RTTR_BINARY_DIR "${RTTR_CONTRIB_DIR}/bin/${CMAKE_LIBRARY_ARCHITECTURE}")
	if(NOT EXISTS "${RTTR_BINARY_DIR}/libcurl.dll")
		MESSAGE(WARNING "Folder with DLLs not found in ${RTTR_BINARY_DIR}. You may not be able to execute directly from VS")
        unset(RTTR_BINARY_DIR)
	ENDIF()
else()
    unset(RTTR_BINARY_DIR)
ENDIF()


include(s25Main.cmake)
add_subdirectory(s25client)
add_subdirectory(test)

if(MSVC)
	CMAKE_POLICY(SET CMP0026 OLD) # Required for use of LOCATION_*
	INCLUDE(CreateLaunchers)
	CREATE_DEFAULT_TARGET_LAUNCHER(s25client ENVIRONMENT RTTR_PREFIX_DIR=${CMAKE_BINARY_DIR})
ELSE()
	SET_TARGET_PROPERTIES(s25client PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${RTTR_BINDIR}"
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${RTTR_BINDIR}"
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/${RTTR_BINDIR}"
	)
ENDIF()
