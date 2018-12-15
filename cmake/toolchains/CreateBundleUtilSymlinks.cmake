# Foreach parameter (binary) create a symlink with everything after the last dash (-) as the name
function(create_bundle_util_symlinks)
  set(symlinkDir ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/bintools)
  file(MAKE_DIRECTORY ${symlinkDir})
  list(APPEND CMAKE_PROGRAM_PATH ${symlinkDir})
  set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} CACHE STRING "")

  foreach(toolPath ${ARGV})
    message(STATUS "Checking ${toolPath}")
    if(toolPath)
      get_filename_component(fileName ${toolPath} NAME)
      string(REGEX MATCH "[^-]+$" baseName ${fileName})
      if(NOT baseName)
        message(FATAL_ERROR "${toolPath} is not a valid tool")
      endif()
      set(symlink ${symlinkDir}/${baseName})
      message(STATUS "Creating symlink ${toolPath} -> ${symlink}")
      if(NOT EXISTS ${symlink})
        execute_process(COMMAND ${CMAKE_COMMAND} -E
          create_symlink ${toolPath} ${symlinkDir}/${baseName}
          RESULT_VARIABLE result
          OUTPUT_VARIABLE out
          ERROR_VARIABLE out
        )
        if(NOT result EQUAL "0")
          message(FATAL_ERROR "Could not create symlink ${symlink}: ${out}")
        endif()
      endif()
    endif()
  endforeach()
endfunction()
