# Meta target that depends on all drivers
add_custom_target(drivers)

# Minimal visibility to avoid name clashes among plugins
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)
#set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_subdirectory(audioDrivers)
add_subdirectory(videoDrivers)
if(RTTR_BUNDLE AND APPLE)
    add_subdirectory(macosLauncher)
endif()
