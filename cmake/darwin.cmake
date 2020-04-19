# Detect possible platforms
include(DetectOsXArchs)
detect_osx_archs()

# Set default to current (if not set)
SET(CHOOSEN_OSX_ARCHS "${CMAKE_OSX_ARCHITECTURES}")
IF(NOT CHOOSEN_OSX_ARCHS)
    if(NOT CMAKE_SYSTEM_PROCESSOR OR CMAKE_SYSTEM_PROCESSOR STREQUAL "universal")
        SET(CHOOSEN_OSX_ARCHS "${OSX_DETECTED_ARCHS}") # Use all
    elseif(CMAKE_SYSTEM_PROCESSOR IN_LIST OSX_DETECTED_ARCHS)
        SET(CHOOSEN_OSX_ARCHS "${CMAKE_SYSTEM_PROCESSOR}")
    else() # Should not happen
        message(FATAL_ERROR "CMAKE_OSX_ARCHITECTURES is not set and CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} is not in the available architectures: ${OSX_DETECTED_ARCHS}")
    endif()
ENDIF()

list(REMOVE_DUPLICATES CHOOSEN_OSX_ARCHS)
list(FIND OSX_DETECTED_ARCHS "i386" _index)
list(FIND OSX_DETECTED_ARCHS "i686" _index2)
if(_index GREATER -1 AND _index2 GREATER -1)
    list(REMOVE_ITEM CHOOSEN_OSX_ARCHS i686) # Not both!
endif()
# Filter by flags and print status
foreach(arch IN LISTS CHOOSEN_OSX_ARCHS)
    if(RTTR_NO${arch})
        list(REMOVE_ITEM CHOOSEN_OSX_ARCHS ${arch})
    else()
        message(STATUS "Building architecture ${arch}")
    endif()
endforeach()
SET(CMAKE_OSX_ARCHITECTURES "${CHOOSEN_OSX_ARCHS}" CACHE STRING "OSX-Architectures" FORCE)
set_property(CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS ${OSX_DETECTED_ARCHS})

IF(NOT CMAKE_OSX_ARCHITECTURES)
	MESSAGE(FATAL_ERROR "No OSX architecture selected!")
ENDIF()

IF(CMAKE_OSX_ARCHITECTURES MATCHES "^i.86$") # i386 only?
    set(RTTR_OPTIMIZATION_TUNE_DEFAULT prescott)
elseIF(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64") # x86_64 only?
    set(RTTR_OPTIMIZATION_TUNE_DEFAULT core2)
ENDIF()

# set linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fexceptions")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fexceptions")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fexceptions")
