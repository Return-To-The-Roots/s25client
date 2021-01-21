# Start script
if(WIN32)
    install(PROGRAMS "bin/rttr.bat" DESTINATION "${RTTR_BINDIR}")
    if(PLATFORM_ARCH)
        include(ConfigureExecutable)
        configure_executable("bundleWinFiles.sh.cmake" . bundleWinFiles.sh)

        install(CODE "
            set(ENV{CMAKE_INSTALL_PREFIX} \${CMAKE_INSTALL_PREFIX})
            execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bundleWinFiles.sh RESULT_VARIABLE result)
            if(NOT result EQUAL \"0\")
              message(FATAL_ERROR \"Failed preparing the release\")
            endif()
        ")
    endif()
elseif(APPLE)
    if(CMAKE_CROSSCOMPILING)
        # copy Frameworks
        foreach(lib IN LISTS SDL_LIBRARY SDL2_LIBRARY SDL_MIXER_LIBRARY)
            if(lib MATCHES "\\.framework$")
                install(DIRECTORY ${lib}
                    DESTINATION ${RTTR_MACOS_DIR}/../Frameworks
                    PATTERN "Headers" EXCLUDE
                )
            endif()
        endforeach()
        # copy miniupnpc
        get_filename_component(MINIUPNPC_DIR ${MINIUPNPC_LIBRARY} DIRECTORY)
        get_filename_component(MINIUPNPC_NAME ${MINIUPNPC_LIBRARY} NAME_WE)
        file(GLOB MINIUPNPC_DYLIBS ${MINIUPNPC_DIR}/${MINIUPNPC_NAME}*.dylib)
        foreach(lib IN LISTS MINIUPNPC_DYLIBS)
            install(FILES ${lib} DESTINATION ${RTTR_LIBDIR})
        endforeach()
    else()
        set(APP "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}")
        set(DIRS "${CMAKE_BINARY_DIR}")
        # Path used for searching by find_xxx(), with appropriate suffixes added
        if(CMAKE_PREFIX_PATH)
            foreach(dir ${CMAKE_PREFIX_PATH})
                list(APPEND DIRS "${dir}/bin" "${dir}/lib")
            endforeach()
        endif()
        install(CODE "
            include(BundleUtilities)
            set(BU_CHMOD_BUNDLE_ITEMS ON)
            file(GLOB_RECURSE LIBS \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${RTTR_DRIVERDIR}/*.*)
            fixup_bundle(\"${APP}\" \"\${LIBS}\" \"${DIRS}\")"
        )
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CYGWIN)
    install(PROGRAMS "bin/rttr.sh" DESTINATION "${RTTR_BINDIR}")
    # copy miniupnpc
    get_filename_component(MINIUPNPC_DIR ${MINIUPNPC_LIBRARY} DIRECTORY)
    find_library(MINIUPNPC_BIN NAMES miniupnpc.so libminiupnpc.so HINTS ${MINIUPNPC_DIR})
    if(MINIUPNPC_BIN)
        get_filename_component(MINIUPNPC_BIN_RES ${MINIUPNPC_BIN} REALPATH)
        install(FILES ${MINIUPNPC_BIN_RES} DESTINATION ${RTTR_LIBDIR})
    else()
        message(WARNING "Could not find miniupnpc lib to install")
    endif()
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
endif()
