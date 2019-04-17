add_library(videoMockup STATIC MockupVideoDriver.h MockupVideoDriver.cpp)
target_link_libraries(videoMockup PUBLIC videodrv s25util PRIVATE nowide::static)
target_include_directories(videoMockup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(audioMockup STATIC MockupAudioDriver.h MockupAudioDriver.cpp)
target_link_libraries(audioMockup PUBLIC audiodrv)
target_include_directories(audioMockup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
