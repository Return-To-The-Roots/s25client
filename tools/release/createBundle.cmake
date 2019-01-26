# Restore variables from original CMake run
set(WIN32 @WIN32@)
set(UNIX @UNIX@)
set(APPLE @APPLE@)

set(CMAKE_OSX_SYSROOT @CMAKE_OSX_SYSROOT@)
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES @CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES@)
set(CMAKE_PREFIX_PATH @CMAKE_PREFIX_PATH@)
set(CMAKE_PROGRAM_PATH @CMAKE_PROGRAM_PATH@)
set(CMAKE_LIBRARY_PATH @CMAKE_LIBRARY_PATH@)
set(CMAKE_MODULE_PATH @CMAKE_MODULE_PATH@)

set(CMAKE_BINARY_DIR @CMAKE_BINARY_DIR@)
set(RTTR_BINDIR @RTTR_BINDIR@)
set(RTTR_DRIVERDIR @RTTR_DRIVERDIR@)
set(Boost_LIBRARY_DIR_RELEASE @Boost_LIBRARY_DIR_RELEASE@)

set(CMAKE_FIND_ROOT_PATH @CMAKE_FIND_ROOT_PATH@)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM @CMAKE_FIND_ROOT_PATH_MODE_PROGRAM@)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY @CMAKE_FIND_ROOT_PATH_MODE_LIBRARY@)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE @CMAKE_FIND_ROOT_PATH_MODE_INCLUDE@)

include(BundleUtilities)

# Hook to adjust decision of file types
function(gp_resolved_file_type_override file type_var)
  if(WIN32)
    set(systemDLLs GDI32 KERNEL32 msvcrt OPENGL32 USER32 ADVAPI32 IPHLPAPI ole32
      SHELL32 WS2_32 WINMM CRYPT32 wldap32 IMM32 OLEAUT32 VERSION)
    get_filename_component(fileName ${file} NAME_WE)
    string(TOLOWER ${fileName} fileName)
    foreach(systemDLL IN LISTS systemDLLs)
      string(TOLOWER ${systemDLL} dll)
      if(fileName STREQUAL dll)
        set(${type_var} system PARENT_SCOPE)
        return()
      endif()
    endforeach()
  endif()
endfunction()

function(gp_resolve_item_override context item exepath dirs resolved_item_var resolved_var)
  # Declare unresolved system libs as resolved
  if(NOT ${resolved_var})
    set(type "")
    gp_resolved_file_type_override(${item} type)
    if(type STREQUAL "system")
      set(${resolved_var} "ON" PARENT_SCOPE)
    endif()
  endif()
endfunction()

# GetPrerequisites does not handle non-default PATH -.-
set(ENV{PATH} $ENV{PATH} ${CMAKE_PROGRAM_PATH})

# Path to application or bundle folder
set(APP "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
if(NOT APPLE OR NOT APP MATCHES "\\.app$")
    get_bundle_all_executables(${APP} executables)
    foreach(exe IN LISTS executables)
      if(exe MATCHES "s25client(\\.[a-z]*)?$")
        set(APP ${exe})
        break()
      endif()
    endforeach()
endif()
# "plugin" libraries (dynamically loaded)
file(GLOB_RECURSE LIBS $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${RTTR_DRIVERDIR}/*.*)
# Paths used to resolved dependencies
set(DIRS ${CMAKE_BINARY_DIR} ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH} ${Boost_LIBRARY_DIR_RELEASE})
# Add prefix paths with suffixes
foreach(dir IN LISTS CMAKE_PREFIX_PATH)
    list(APPEND DIRS "${dir}/bin" "${dir}/lib")
endforeach()

set(BU_CHMOD_BUNDLE_ITEMS ON)
fixup_bundle("${APP}" "${LIBS}" "${DIRS}")
