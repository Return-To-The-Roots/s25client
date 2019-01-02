set(SDL2_BUILDING_LIBRARY ON)
find_package(SDL2 2.0.4)

if(SDL2_FOUND)
	add_library(videoSDL2 SHARED ${RTTR_DRIVER_INTERFACE} VideoSDL2.cpp VideoSDL2.h icon.h icon.cpp)
    target_link_libraries(videoSDL2 PRIVATE videodrv s25util glad utf8 SDL2::SDL2)

	if(WIN32)
        include(GatherDll)
        gather_dll_by_name(SDL2 SDL2.dll)
	endif()

	install(TARGETS videoSDL2 DESTINATION ${RTTR_DRIVERDIR}/video)
endif()
