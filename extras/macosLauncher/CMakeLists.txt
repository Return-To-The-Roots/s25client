if(NOT RTTR_OSX_BUNDLE OR NOT RTTR_MACOS_DIR)
    message(FATAL_ERROR "Can only use this in a bundle")
endif()

add_executable(starter starter.cpp)
target_link_libraries(starter PRIVATE s25util rttrConfig nowide-static)
set(MACOSX_BUNDLE_EXECUTABLE_NAME "starter${CMAKE_EXECUTABLE_SUFFIX}")
set(MACOSX_BUNDLE_BUNDLE_VERSION "${RTTR_VERSION}")
set(MACOSX_BUNDLE_COPYRIGHT "(C) 2005-2018 Settlers Freaks")
set(MACOSX_BUNDLE_LONG_VERSION_STRING "${RTTR_REVISION}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${RTTR_VERSION}")
configure_file(Info.plist.in ${PROJECT_BINARY_DIR}/Info.plist @ONLY)
install(TARGETS starter DESTINATION ${RTTR_BINDIR})
install(FILES rttr.terminal DESTINATION ${RTTR_BINDIR})
install(FILES ${PROJECT_BINARY_DIR}/Info.plist DESTINATION ${RTTR_MACOS_DIR}/..)
install(FILES icon.icns DESTINATION ${RTTR_MACOS_DIR}/../Resources)

# To be invoked later
function(rttr_create_bundle)
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
        # Path used for searching by FIND_XXX(), with appropriate suffixes added
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
endfunction()
