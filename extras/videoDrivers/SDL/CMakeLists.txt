set(SDL_BUILDING_LIBRARY ON)
find_package(SDL)

if(SDL_FOUND)
  find_package(Boost 1.64.0 REQUIRED)
  include(CorrectLib)
  correct_lib(SDL_LIBRARY SDL)

  add_library(videoSDL SHARED ${RTTR_DRIVER_INTERFACE} VideoSDL.cpp VideoSDL.h)
  target_link_libraries(videoSDL PRIVATE videodrv s25util ${SDL_LIBRARY} Boost::boost nowide::static)
  target_include_directories(videoSDL PRIVATE ${SDL_INCLUDE_DIR})

  if(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(videoSDL PRIVATE ${OPENGL_gl_LIBRARY})
    target_include_directories(videoSDL PRIVATE ${OPENGL_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/data/win32)
    include(GatherDll)
    gather_dll_by_name(SDL SDL.dll)
  endif()

  install(TARGETS videoSDL
    RUNTIME DESTINATION ${RTTR_DRIVERDIR}/video
    LIBRARY DESTINATION ${RTTR_DRIVERDIR}/video
  )
  add_dependencies(drivers videoSDL)
else()
  message(WARNING "SDL library not found: Not building SDL videodriver")
endif()
