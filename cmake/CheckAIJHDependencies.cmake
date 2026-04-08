# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

if(NOT DEFINED ROOT)
    get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(VIOLATIONS "")

macro(collect_domain_files out_var rel_dir)
    file(GLOB_RECURSE ${out_var}
        "${ROOT}/${rel_dir}/*.h"
        "${ROOT}/${rel_dir}/*.cpp"
    )
endmacro()

macro(record_violation rel_file message)
    string(APPEND VIOLATIONS "${rel_file}: ${message}\n")
endmacro()

macro(check_no_pattern rel_dir regex description)
    collect_domain_files(_check_files "${rel_dir}")
    foreach(_check_file IN LISTS _check_files)
        file(STRINGS "${_check_file}" _matches REGEX "${regex}")
        if(_matches)
            file(RELATIVE_PATH _rel_file "${ROOT}" "${_check_file}")
            foreach(_match IN LISTS _matches)
                string(STRIP "${_match}" _match)
                record_violation("${_rel_file}" "${description}: ${_match}")
            endforeach()
        endif()
    endforeach()
endmacro()

macro(check_allowed_runtime_includes rel_dir)
    set(_allowed_runtime_headers ${ARGN})
    collect_domain_files(_check_files "${rel_dir}")
    foreach(_check_file IN LISTS _check_files)
        file(STRINGS "${_check_file}" _runtime_includes REGEX "^#include \"ai/aijh/runtime/[^\"]+\"")
        foreach(_runtime_include IN LISTS _runtime_includes)
            string(REGEX REPLACE "^#include \"ai/aijh/runtime/([^\"]+)\".*$" "\\1" _runtime_header "${_runtime_include}")
            list(FIND _allowed_runtime_headers "${_runtime_header}" _runtime_header_index)
            if(_runtime_header_index EQUAL -1)
                file(RELATIVE_PATH _rel_file "${ROOT}" "${_check_file}")
                record_violation("${_rel_file}" "unexpected runtime include: ${_runtime_header}")
            endif()
        endforeach()
    endforeach()
endmacro()

check_no_pattern("libs/s25main/ai/aijh/planning" "AIPlayerJH" "planning must not mention AIPlayerJH")
check_allowed_runtime_includes("libs/s25main/ai/aijh/planning" "AIPlanningContext.h" "AIWorldView.h")

check_no_pattern("libs/s25main/ai/aijh/combat" "AIPlayerJH" "combat must not mention AIPlayerJH")
check_no_pattern("libs/s25main/ai/aijh/combat" "^#include \"ai/aijh/planning/[^\"]+\"" "combat must not include planning")
check_no_pattern("libs/s25main/ai/aijh/combat" "^#include \"ai/aijh/runtime/[^\"]+\"" "combat must not include runtime headers directly")

check_no_pattern("libs/s25main/ai/aijh/debug" "AIPlayerJH" "debug must not mention AIPlayerJH")
check_allowed_runtime_includes("libs/s25main/ai/aijh/debug" "AIMap.h")
check_no_pattern(
    "libs/s25main/ai/aijh/debug"
    "^#include \"ai/aijh/runtime/(AIPlanningContext|AIPlayerJH|AIPlayerJHInternal|AIEconomyController|AIEventHandler|AIMapState|AIMilitaryLogistics|AIRoadController|AIWorldQueries)[.]h\""
    "debug must not include runtime implementation headers"
)

if(VIOLATIONS)
    message(FATAL_ERROR "AI JH dependency guard failed:\n${VIOLATIONS}")
endif()

message(STATUS "AI JH dependency guards passed")
