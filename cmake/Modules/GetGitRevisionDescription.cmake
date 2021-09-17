# SPDX-License-Identifier: BSL-1.0

# - Returns a version string from Git
#
# These functions force a re-configure on each git commit so that you can
# trust the values of the variables in your build system.
#
#  get_git_head_revision(<refspecvar> <hashvar>)
#
# Returns the refspec and sha hash of the current head revision
#
#  git_describe(<var> [<additional arguments to git describe> ...])
#
# Returns the results of git describe on the source tree, and adjusting
# the output so that it tests false if an error occurs.
#
#  git_get_exact_tag(<var> [<additional arguments to git describe> ...])
#
# Returns the results of git describe --exact-match on the source tree,
# and adjusting the output so that it tests false if there was no exact
# matching tag.
#
#  git_local_changes(<var>)
#
# Returns either "CLEAN" or "DIRTY" with respect to uncommitted changes.
# Uses the return code of "git diff-index --quiet HEAD --".
# Does not regard untracked files.
#
# Requires CMake 2.6 or newer (uses the 'function' command)
#
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2009-2010.

function(_find_git_dir git_dir_var)
    set(GIT_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(git_dir "${GIT_PARENT_DIR}/.git")
    while(NOT EXISTS "${git_dir}")  # .git dir not found, search parent directories
        set(GIT_PREVIOUS_PARENT "${GIT_PARENT_DIR}")
        get_filename_component(GIT_PARENT_DIR ${GIT_PARENT_DIR} PATH)
        if(GIT_PARENT_DIR STREQUAL GIT_PREVIOUS_PARENT)
            # We have reached the root directory, we are not in git
            set(${git_dir_var} "GITDIR-NOTFOUND" PARENT_SCOPE)
            return()
        endif()
        set(git_dir "${GIT_PARENT_DIR}/.git")
    endwhile()
    # check if this is a submodule
    if(NOT IS_DIRECTORY ${git_dir})
        file(READ ${git_dir} submodule)
        string(REGEX REPLACE "gitdir: (.*)\n$" "\\1" GIT_DIR_RELATIVE ${submodule})
        get_filename_component(SUBMODULE_DIR ${git_dir} PATH)
        get_filename_component(git_dir ${SUBMODULE_DIR}/${GIT_DIR_RELATIVE} ABSOLUTE)
    endif()
    if(NOT EXISTS "${git_dir}/HEAD")
        set(${git_dir_var} "GITHEAD-NOTFOUND" PARENT_SCOPE)
        return()
    endif()
    set(${git_dir_var} "${git_dir}" PARENT_SCOPE)
endfunction()

function(get_git_head_revision _refspecvar _hashvar)
    _find_git_dir(git_dir)
    if(NOT git_dir)
        set(${_refspecvar} "${git_dir}" PARENT_SCOPE)
        set(${_hashvar} "${git_dir}" PARENT_SCOPE)
        return()
    endif()

    set(git_data_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/git-data")
    if(NOT EXISTS "${git_data_dir}")
        file(MAKE_DIRECTORY "${git_data_dir}")
    endif()

    set(head_file "${git_data_dir}/HEAD")
    configure_file("${git_dir}/HEAD" "${head_file}" COPYONLY)

    set(HEAD_HASH "")
    file(READ "${head_file}" head_contents LIMIT 1024)
    string(STRIP "${head_contents}" head_contents)
    if(head_contents MATCHES "ref")
        # named branch
        string(REPLACE "ref: " "" HEAD_REF "${head_contents}")
        if(EXISTS "${git_dir}/${HEAD_REF}")
            configure_file("${git_dir}/${HEAD_REF}" "${git_data_dir}/head-ref" COPYONLY)
            file(READ "${git_data_dir}/head-ref" HEAD_HASH LIMIT 1024)
            string(STRIP "${HEAD_HASH}" HEAD_HASH)
        else()
            configure_file("${git_dir}/packed-refs" "${git_data_dir}/packed-refs" COPYONLY)
            file(READ "${git_data_dir}/packed-refs" packed_refs)
            if(packed_refs MATCHES "([0-9a-z]*) ${HEAD_REF}")
                set(HEAD_HASH "${CMAKE_MATCH_1}")
            else()
                message(FATAL_ERROR "${HEAD_REF} not found in packed-refs")
            endif()
        endif()
    else()
        # detached HEAD
        set(HEAD_REF "detached")
        file(READ "${head_file}" HEAD_HASH LIMIT 1024)
        string(STRIP "${HEAD_HASH}" HEAD_HASH)
    endif()

    set(${_refspecvar} "${HEAD_REF}" PARENT_SCOPE)
    set(${_hashvar} "${HEAD_HASH}" PARENT_SCOPE)
endfunction()

function(git_describe _var)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
        if(NOT GIT_FOUND)
            set(${_var} "GIT-NOTFOUND" PARENT_SCOPE)
            return()
        endif()
    endif()
    get_git_head_revision(refspec hash)
    if(NOT hash)
        set(${_var} "HEAD-HASH-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    execute_process(COMMAND "${GIT_EXECUTABLE}" describe ${hash} ${ARGN}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      RESULT_VARIABLE res
      OUTPUT_VARIABLE out
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT res EQUAL 0)
        set(out "${out}-${res}-NOTFOUND")
    endif()

    set(${_var} "${out}" PARENT_SCOPE)
endfunction()

function(git_get_exact_tag _var)
    git_describe(out --exact-match ${ARGN})
    set(${_var} "${out}" PARENT_SCOPE)
endfunction()

function(git_local_changes _var)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
        if(NOT GIT_FOUND)
            set(${_var} "GIT-NOTFOUND" PARENT_SCOPE)
            return()
        endif()
    endif()
    get_git_head_revision(refspec hash)
    if(NOT hash)
        set(${_var} "HEAD-HASH-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    execute_process(COMMAND "${GIT_EXECUTABLE}" diff-index --quiet HEAD --
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      RESULT_VARIABLE res
      OUTPUT_VARIABLE out
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(res EQUAL 0)
        set(${_var} "CLEAN" PARENT_SCOPE)
    else()
        set(${_var} "DIRTY" PARENT_SCOPE)
    endif()
endfunction()
