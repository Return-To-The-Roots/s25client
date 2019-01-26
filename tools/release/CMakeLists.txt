set(bundleScript ${CMAKE_CURRENT_BINARY_DIR}/createBundle.cmale)
configure_file(createBundle.cmake ${bundleScript} @ONLY)
install(SCRIPT ${bundleScript})

# Start script
if(WIN32)
    install(PROGRAMS "bin/rttr.bat" DESTINATION "${RTTR_BINDIR}")
elseif(APPLE)
    # Nothing
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CYGWIN)
    install(PROGRAMS "bin/rttr.sh" DESTINATION "${RTTR_BINDIR}")
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
endif()
