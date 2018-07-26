IF (WIN32)
	FIND_PACKAGE(Boost 1.55.0 REQUIRED)
    find_package(OpenGL REQUIRED)

	ADD_LIBRARY(videoWinAPI SHARED WinAPI.cpp WinAPI.h ${PROJECT_SOURCE_DIR}/driver/src/DriverInterface.cpp)
	TARGET_LINK_LIBRARIES(videoWinAPI videodrv s25util gdi32 user32 ${OPENGL_gl_LIBRARY}) 
    target_include_directories(videoWinAPI PRIVATE ${Boost_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    target_compile_definitions(videoWinAPI PRIVATE -DBUILD_DLL)

	INSTALL(TARGETS videoWinAPI RUNTIME DESTINATION ${RTTR_DRIVERDIR}/video)
ENDIF ()
