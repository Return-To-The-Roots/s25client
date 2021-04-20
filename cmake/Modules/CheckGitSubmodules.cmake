# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

function(check_git_submodules)
  find_package(Git QUIET)
  if(NOT GIT_FOUND)
    message(WARNING "Git could not be found. You have to manually make sure all sources and submodules are available.")
    return()
  endif()
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule status
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  RESULT_VARIABLE result
                  OUTPUT_VARIABLE output)
  if(NOT result EQUAL "0")
    message(WARNING "Could not get status of git submodules")
  endif()
  set(missingModules )
  set(changedModules )
  # Convert to CMake list
  string(REPLACE ";" "\\;" output "${output}")
  string(REPLACE "\n" ";" output "${output}")
  foreach(line ${output})
    if(${line} MATCHES "(\\+|-)?[a-z0-9]+ ([^ ]+) ")
      if(CMAKE_MATCH_1 STREQUAL "+")
        list(APPEND changedModules ${CMAKE_MATCH_2})
      elseif(CMAKE_MATCH_1 STREQUAL "-")
        list(APPEND missingModules ${CMAKE_MATCH_2})
      endif()
    endif()
  endforeach()
  if(missingModules)
    message(WARNING "Git modules '${missingModules}' are missing. You need to run `git submodule update --init`")
  elseif(changedModules)
    message(WARNING "Git modules '${changedModules}' were changed. You may need to run `git submodule update` if you are not actively developing them.")
  endif()
endfunction()
