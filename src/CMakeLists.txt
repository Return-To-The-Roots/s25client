
FIND_PACKAGE(BZip2 REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Gettext REQUIRED)
FIND_PACKAGE(Boost 1.55.0 COMPONENTS filesystem iostreams system program_options locale REQUIRED)

INCLUDE(CMakeMacroForceAddFlags)
INCLUDE(CMakeMacroRemoveFlags)

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

FIND_PACKAGE(LUA REQUIRED)

INCLUDE_DIRECTORIES(SYSTEM ${LUA_INCLUDE_DIR})

SET(RTTR_Assert_Enabled 2 CACHE STRING "Status of RTTR assertions: 0=Disabled, 1=Enabled, 2=Default(Enabled only in debug)")
IF("${RTTR_Assert_Enabled}" EQUAL 0)
	ADD_DEFINITIONS(-DRTTR_ENABLE_ASSERTS=0)
ELSEIF("${RTTR_Assert_Enabled}" EQUAL 1)
	ADD_DEFINITIONS(-DRTTR_ENABLE_ASSERTS=1)
ENDIF()

################################################################################

unset(RTTR_BINARIES_TO_COPY)
if(MSVC)
	# disable warning 4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
	ADD_DEFINITIONS(/wd4267)
    option(RTTR_EDITANDCONTINUE "Enable Edit-And-Continue" OFF)
    if(RTTR_EDITANDCONTINUE)
        # Enable edit-and-continue
        FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_RELWITHDEBINFO /ZI)
        REMOVE_FLAGS(CMAKE_CXX_FLAGS_RELWITHDEBINFO /Zi)
        FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_DEBUG /ZI)
        REMOVE_FLAGS(CMAKE_CXX_FLAGS_DEBUG /Zi)
    else()
        FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_RELWITHDEBINFO /Zi)
        REMOVE_FLAGS(CMAKE_CXX_FLAGS_RELWITHDEBINFO /ZI)
        FORCE_ADD_FLAGS(CMAKE_CXX_FLAGS_DEBUG /Zi)
        REMOVE_FLAGS(CMAKE_CXX_FLAGS_DEBUG /ZI)
    endif()
    FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS_DEBUG /SAFESEH:NO)
    FORCE_ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO /SAFESEH:NO)

 	SET(RTTR_BINARY_DIR "${RTTR_CONTRIB_DIR}/bin/${CMAKE_LIBRARY_ARCHITECTURE}")
	if(NOT EXISTS "${RTTR_BINARY_DIR}/libcurl.dll")
		MESSAGE(WARNING "Folder with DLLs not found in ${RTTR_BINARY_DIR}. You may not be able to execute directly from VS")
    else()
        FILE(GLOB RTTR_BINARIES_TO_COPY ${RTTR_BINARY_DIR}/*.*)
        LIST(APPEND RTTR_BINARIES_TO_COPY ${LUA_DLL})
	ENDIF()
ENDIF()


include(s25Main.cmake)
add_subdirectory(s25client)
add_subdirectory(test)
