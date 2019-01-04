find_package(SDL 1.2 REQUIRED)

include(CorrectLib)
correct_lib(SDL_LIBRARY SDL)

add_library(videoMockup STATIC MockupVideoDriver.h MockupVideoDriver.cpp)
target_link_libraries(videoMockup PUBLIC videodrv s25util PRIVATE ${SDL_LIBRARY} nowide::static)
target_include_directories(videoMockup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(videoMockup SYSTEM PUBLIC ${SDL_INCLUDE_DIR})

add_library(audioMockup STATIC MockupAudioDriver.h MockupAudioDriver.cpp)
target_link_libraries(audioMockup PUBLIC audiodrv)
target_include_directories(audioMockup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(WIN32)
	include(GatherDll)
	gather_dll_by_name(SDL SDL.dll)
endif()
