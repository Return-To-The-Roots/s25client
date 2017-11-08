FIND_PACKAGE(BZip2 REQUIRED)
gather_dll(BZIP2)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Gettext REQUIRED)
FIND_PACKAGE(Boost 1.55.0 REQUIRED COMPONENTS filesystem iostreams system program_options locale thread)

INCLUDE(CMakeMacroForceAddFlags)
INCLUDE(CMakeMacroRemoveFlags)

################################################################################

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/driver/include)
INCLUDE_DIRECTORIES(SYSTEM
	${BZIP2_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${RTTR_CONTRIB_DIR}/kaguya/include
	)

################################################################################

FILE(GLOB RTTR_PO_FILES ../RTTR/languages/*.po)

GETTEXT_CREATE_TRANSLATIONS(../RTTR/languages/rttr.pot ALL
							${RTTR_PO_FILES}
						   )

################################################################################

if(APPLE)
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

include(GatherDll)
gather_dll(LUA)

################################################################################
# OpenGL
################################################################################

find_path(OGLEXT_INCLUDE_DIR NAMES GL/glext.h OpenGL/glext.h)
if(NOT OGLEXT_INCLUDE_DIR)
    set(OGLEXT_INCLUDE_DIR ${RTTR_CONTRIB_DIR}/openGL/include CACHE PATH "" FORCE)
elseif(WIN32)
    # On windows the ARB function signatures might not be defined due to an old glExt version
    # So if they are not we use our new ones
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${OGLEXT_INCLUDE_DIR})
    check_cxx_source_compiles("
        #include <windows.h>
        #include <GL/gl.h>
        #include <GL/glext.h>
        PFNGLGENBUFFERSARBPROC test;
        int main(){}" OGL_ARB_FUNCS_EXIST)
    set(CMAKE_REQUIRED_INCLUDES )
    if(NOT OGL_ARB_FUNCS_EXIST)
        set(OGLEXT_INCLUDE_DIR ${RTTR_CONTRIB_DIR}/openGL/include CACHE PATH "" FORCE)
    endif()
endif()
include_directories(BEFORE SYSTEM ${OGLEXT_INCLUDE_DIR})

################################################################################

SET(RTTR_Assert_Enabled 2 CACHE STRING "Status of RTTR assertions: 0=Disabled, 1=Enabled, 2=Default(Enabled only in debug)")
IF("${RTTR_Assert_Enabled}" EQUAL 0)
	ADD_DEFINITIONS(-DRTTR_ENABLE_ASSERTS=0)
ELSEIF("${RTTR_Assert_Enabled}" EQUAL 1)
	ADD_DEFINITIONS(-DRTTR_ENABLE_ASSERTS=1)
ENDIF()

if(MSVC)
	# disable warning 4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
	add_compile_options(/wd4267)
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
ENDIF()


include(s25Main.cmake)
add_subdirectory(s25client)
add_subdirectory(test)

# Copy the RTTR folder with the updated translations to the directory of the client.
ADD_CUSTOM_COMMAND(TARGET translations POST_BUILD COMMAND ${CMAKE_COMMAND} -D CMAKE_BUILD_TYPE=$<CONFIGURATION> -P ${CMAKE_BINARY_DIR}/CMakeFiles/copyDepsToBuildDir.cmake)
