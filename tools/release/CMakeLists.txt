if(NOT CMAKE_HOST_WIN32 AND PLATFORM_ARCH)
    if(WIN32)
        include(ConfigureExecutable)
        configure_executable("prepareRelease.sh.cmake" . prepareRelease.sh)

        install(CODE "
            set(ENV{CMAKE_INSTALL_PREFIX} \${CMAKE_INSTALL_PREFIX})
            execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/prepareRelease.sh)
        ")
    endif()
endif()
