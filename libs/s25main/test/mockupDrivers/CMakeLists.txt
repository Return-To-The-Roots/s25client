FIND_PACKAGE(SDL 1.2 REQUIRED)
INCLUDE(CMakeMacroCorrectLib)
	
CORRECT_LIB(SDL_LIBRARY SDL)

include_directories(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(SYSTEM ${SDL_INCLUDE_DIR})

IF (WIN32)
	include(GatherDll)
	gather_dll_by_name(SDL SDL.dll)
ENDIF ()

ADD_LIBRARY(videoMockup STATIC MockupVideoDriver.h MockupVideoDriver.cpp)
TARGET_LINK_LIBRARIES(videoMockup PUBLIC videodrv s25util ${SDL_LIBRARY})

ADD_LIBRARY(audioMockup STATIC MockupAudioDriver.h MockupAudioDriver.cpp)
TARGET_LINK_LIBRARIES(audioMockup PUBLIC audiodrv)

if(ClangFormat_FOUND)
    add_clangFormat_files(MockupVideoDriver.h MockupVideoDriver.cpp MockupAudioDriver.h MockupAudioDriver.cpp)
endif()
