# CMake < 3.4 does not know about tbd and dylib for shared libraries
IF(CMAKE_VERSION VERSION_LESS 3.4)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".tbd;.dylib;.so;.a" CACHE INTERNAL "Library Suffixes")
ENDIF()

# Detect possible platforms
include(CMakeDetectOsXArchs)
DetectOSXArchs()

# Set default to current (if not set)
SET(CHOOSEN_OSX_ARCHS "${CMAKE_OSX_ARCHITECTURES}")
IF(NOT CHOOSEN_OSX_ARCHS)
    # TODO if(... IN_LIST ...) at CMake 3.3
    list(FIND OSX_POSSIBLE_ARCHS "${CMAKE_SYSTEM_PROCESSOR}" _index)
    if(_index GREATER -1)
        SET(CHOOSEN_OSX_ARCHS "${CMAKE_SYSTEM_PROCESSOR}")
    elseif(NOT CMAKE_SYSTEM_PROCESSOR OR CMAKE_SYSTEM_PROCESSOR STREQUAL "universal")
        SET(CHOOSEN_OSX_ARCHS "${OSX_POSSIBLE_ARCHS}") # Use all
    else() # Should not happen
        message(FATAL_ERROR "CMAKE_OSX_ARCHITECTURES is not set and CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} is not in the available architectures: ${OSX_POSSIBLE_ARCHS}")
    endif()
ENDIF()

list(REMOVE_DUPLICATES CHOOSEN_OSX_ARCHS)
list(FIND OSX_POSSIBLE_ARCHS "i386" _index)
list(FIND OSX_POSSIBLE_ARCHS "i686" _index2)
if(_index GREATER -1 AND _index2 GREATER -1)
    list(REMOVE_ITEM CHOOSEN_OSX_ARCHS i686) # Not both!
endif()
# Filter by flags and print status
foreach(arch IN LISTS CHOOSEN_OSX_ARCHS)
    if(RTTR_NO${arch})
        list(REMOVE_ITEM CHOOSEN_OSX_ARCHS ${arch})
    endif()
    message(STATUS "Building architecture ${arch}")
endforeach()
SET(CMAKE_OSX_ARCHITECTURES "${CHOOSEN_OSX_ARCHS}" CACHE STRING "OSX-Architectures" FORCE)
set_property(CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS ${OSX_POSSIBLE_ARCHS})

IF(NOT CMAKE_OSX_ARCHITECTURES)
	MESSAGE(FATAL_ERROR "No OSX architecture selected!")
ENDIF()

IF(CMAKE_OSX_ARCHITECTURES MATCHES "^i.86$") # i386 only?
    set(RTTR_OPTIMZATION_TUNE_DEFAULT prescott)
elseIF(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64") # x86_64 only?
    set(RTTR_OPTIMZATION_TUNE_DEFAULT core2)
ENDIF()
# set linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fexceptions")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fexceptions")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fexceptions")
