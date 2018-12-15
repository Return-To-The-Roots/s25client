# Need to be in extra dir to be included after all installation commands
# Otherwise install(CODE) is run before installation of files is done
if(APPLE)
    if(FALSE AND CMAKE_CROSSCOMPILING)
        # copy Frameworks
        foreach(lib IN LISTS SDL_LIBRARY SDL_MIXER_LIBRARY)
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
        # Path used for searching by FIND_XXX(), with appropriate suffixes added
        if(CMAKE_PREFIX_PATH)
            foreach(dir ${CMAKE_PREFIX_PATH})
                list(APPEND DIRS "${dir}/bin" "${dir}/lib")
            endforeach()
        endif()
        set(varInit)
        foreach(var
            WIN32 UNIX APPLE
            CMAKE_OSX_SYSROOT CMAKE_PREFIX_PATH CMAKE_PROGRAM_PATH CMAKE_LIBRARY_PATH
            CMAKE_FIND_ROOT_PATH CMAKE_FIND_ROOT_PATH_MODE_PROGRAM CMAKE_FIND_ROOT_PATH_MODE_LIBRARY CMAKE_FIND_ROOT_PATH_MODE_INCLUDE
        )
            set(varInit "${varInit}set(${var} ${${var}})\n")
        endforeach()
        install(CODE "
            ${varInit}
            # GetPrerequisites does not handle non-default PATH -.-
            set(PATH \$ENV{PATH})
            foreach(p IN LISTS CMAKE_PROGRAM_PATH)
                list(APPEND PATH \${p})
            endforeach()
            set(ENV{PATH} \${PATH})
            include(BundleUtilities)
            set(BU_CHMOD_BUNDLE_ITEMS ON)
            file(GLOB_RECURSE LIBS \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${RTTR_DRIVERDIR}/*.*)
            fixup_bundle(\"${APP}\" \"\${LIBS}\" \"${DIRS}\")"
        )
    endif()
endif()
