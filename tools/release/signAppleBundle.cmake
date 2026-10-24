# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Ad-hoc signs all Mach-O files of the app bundle and verifies the signatures.
# Must run after everything that modifies the binaries (strip, install_name_tool, ...)
# as those invalidate or remove the signature, which makes arm64 binaries unusable.
#
# Expects: APP_DIR - Path to the (staged) app bundle

if(NOT IS_DIRECTORY "${APP_DIR}")
    message(FATAL_ERROR "Bundle directory '${APP_DIR}' does not exist")
endif()

# Native macOS uses the Apple tooling, otherwise (cross compiling) rcodesign or ldid
if(CMAKE_HOST_APPLE)
    find_program(CODESIGN_BIN codesign)
endif()
# OTOOL_BIN can be passed by the caller (CMAKE_OTOOL of the toolchain)
find_program(OTOOL_BIN NAMES otool llvm-otool)
find_program(RCODESIGN_BIN rcodesign)
find_program(LDID_BIN ldid)
if(NOT CODESIGN_BIN AND NOT RCODESIGN_BIN AND NOT LDID_BIN)
    message(FATAL_ERROR "None of codesign, rcodesign or ldid found: Unable to sign the bundle")
endif()

# Collect all Mach-O files (thin and fat), ignoring data files
file(GLOB_RECURSE allFiles LIST_DIRECTORIES false "${APP_DIR}/*")
set(machoFiles "")
foreach(file IN LISTS allFiles)
    if(IS_SYMLINK "${file}" OR file MATCHES "/share/")
        continue()
    endif()
    file(READ "${file}" magic LIMIT 4 HEX)
    if(magic MATCHES "^(feedface|feedfacf|cefaedfe|cffaedfe|cafebabe|bebafeca)$")
        list(APPEND machoFiles "${file}")
    endif()
endforeach()

if(NOT machoFiles)
    message(FATAL_ERROR "No Mach-O files found in '${APP_DIR}'")
endif()

foreach(file IN LISTS machoFiles)
    file(RELATIVE_PATH relFile "${APP_DIR}" "${file}")
    # Fix identifier instead of whatever the linker used (e.g. "<name>-<arch>.out")
    get_filename_component(identifier "${file}" NAME_WLE)
    if(OTOOL_BIN)
        execute_process(COMMAND "${OTOOL_BIN}" -D "${file}" RESULT_VARIABLE result OUTPUT_VARIABLE installNames ERROR_QUIET)
        if(result EQUAL 0)
            string(REPLACE "\n" ";" installNames "${installNames}")
            foreach(installName IN LISTS installNames)
                # Skip header lines ("<file>:" or "<file> (architecture x):") and empty lines
                if(installName AND NOT installName MATCHES ":$")
                    get_filename_component(installName "${installName}" NAME)
                    string(REGEX REPLACE "\\.dylib$" "" identifier "${installName}")
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(CODESIGN_BIN)
        set(signCmd "${CODESIGN_BIN}" --force --sign - --identifier "${identifier}" "${file}")
        set(infoCmd "${CODESIGN_BIN}" --verify --strict --verbose=2 "${file}")
    elseif(RCODESIGN_BIN)
        set(signCmd "${RCODESIGN_BIN}" sign --binary-identifier "${identifier}" "${file}")
        set(infoCmd "${RCODESIGN_BIN}" print-signature-info "${file}")
    else()
        set(signCmd "${LDID_BIN}" -S "-I${identifier}" "${file}")
        set(infoCmd "${LDID_BIN}" -h "${file}")
    endif()

    # Keep signatures that are still valid (matching identifier and content). strip & co keep stale signatures.
    set(needsSigning TRUE)
    if(CODESIGN_BIN)
        execute_process(COMMAND ${infoCmd} RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
        if(result EQUAL 0)
            set(needsSigning FALSE)
        endif()
    elseif(RCODESIGN_BIN)
        # No reliable verify command: Compare the code hashes stored in the signature with freshly computed ones.
        # Check every architecture slice of universal binaries
        set(needsSigning FALSE)
        set(index 0)
        while(TRUE)
            execute_process(COMMAND "${RCODESIGN_BIN}" compute-code-hashes --universal-index ${index} "${file}"
                RESULT_VARIABLE result OUTPUT_VARIABLE computedHashes ERROR_QUIET)
            if(NOT result EQUAL 0)
                break() # No more slices
            endif()
            execute_process(COMMAND "${RCODESIGN_BIN}" extract --universal-index ${index} code-directory "${file}"
                RESULT_VARIABLE result OUTPUT_VARIABLE codeDirectory ERROR_QUIET)
            # Unsigned or different identifier
            if(NOT result EQUAL 0 OR NOT codeDirectory MATCHES "\n *ident: \"${identifier}\",\n")
                set(needsSigning TRUE)
                break()
            endif()
            set(storedHashes "")
            if(codeDirectory MATCHES "code_digests: \\[([^]]*)\\]")
                string(REGEX MATCHALL "[0-9a-f]+" storedHashes "${CMAKE_MATCH_1}")
            endif()
            string(REGEX MATCHALL "[0-9a-f]+" calcHashes "${computedHashes}")
            if(NOT storedHashes OR NOT storedHashes STREQUAL calcHashes)
                set(needsSigning TRUE)
                break()
            endif()
            math(EXPR index "${index} + 1")
        endwhile()
        if(index EQUAL 0)
            set(needsSigning TRUE)
        endif()
    endif()

    if(needsSigning)
        execute_process(COMMAND ${signCmd} RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Signing ${relFile} failed:\n${out}")
        endif()
    endif()

    # Verify: codesign fails on invalid signatures, the other tools need a CodeDirectory in their output
    execute_process(COMMAND ${infoCmd} RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT result EQUAL 0 OR (NOT CODESIGN_BIN AND NOT out MATCHES "CodeDirectory"))
        message(FATAL_ERROR "Signature verification of ${relFile} failed:\n${out}")
    endif()

    # Show the verification result
    if(RCODESIGN_BIN AND NOT CODESIGN_BIN)
        # One entry per architecture slice
        string(REGEX MATCHALL "identifier: [^\n]+" ids "${out}")
        string(REGEX MATCHALL "flags: CodeSignatureFlags\\([^)]*\\)" flags "${out}")
        string(REGEX MATCHALL "CodeDirectory \\(0\\)\n +magic: [0-9a-f]+\n +length: [0-9]+\n +sha1: [0-9a-f]+\n +sha256: [0-9a-f]+" cdirs "${out}")
        set(info "")
        foreach(cdir IN LISTS cdirs)
            string(REGEX REPLACE ".*sha256: " "" cdhash "${cdir}")
            list(APPEND info "CodeDirectory sha256=${cdhash}")
        endforeach()
        list(APPEND info ${ids} ${flags})
        list(JOIN info ", " info)
    else()
        string(STRIP "${out}" info)
        string(REPLACE "\n" " | " info "${info}")
    endif()
    if(needsSigning)
        message(NOTICE "Signed: ${relFile} (${info})")
    else()
        message(NOTICE "Valid signature kept: ${relFile} (${info})")
    endif()
endforeach()
