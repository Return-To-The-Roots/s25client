if(WIN32)
  find_package(OpenGL REQUIRED)

  add_library(videoWinAPI SHARED ${RTTR_DRIVER_INTERFACE} WinAPI.cpp WinAPI.h)
  target_link_libraries(videoWinAPI PRIVATE videodrv s25util::common gdi32 user32 ${OPENGL_gl_LIBRARY} Boost::boost)
  target_include_directories(videoWinAPI SYSTEM PRIVATE ${OPENGL_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/data/win32)
  enable_warnings(videoWinAPI)

  install(TARGETS videoWinAPI RUNTIME DESTINATION ${RTTR_DRIVERDIR}/video)
  add_dependencies(drivers videoWinAPI)
endif()
