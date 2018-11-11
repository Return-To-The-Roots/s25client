find_package(SDL2 2.0.4)

if(SDL2_FOUND)
	add_library(videoSDL2 SHARED VideoSDL2.cpp VideoSDL2.h ${PROJECT_SOURCE_DIR}/driver/src/DriverInterface.cpp icon.h icon.cpp)
	target_include_directories(videoSDL2 PRIVATE ${SDL2_INCLUDE_DIRS})

	if(WIN32)
		target_compile_definitions(videoSDL2 PRIVATE -DBUILD_DLL)
        include(GatherDll)
        gather_dll_by_name(SDL2 SDL2.dll)
	endif()

	target_link_libraries(videoSDL2 PRIVATE videodrv s25util glad utf8 ${SDL2_LIBRARIES})

	install(TARGETS videoSDL2 DESTINATION ${RTTR_DRIVERDIR}/video)
endif()
