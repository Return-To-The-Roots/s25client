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
    if(CODESIGN_BIN)
        set(signCmd "${CODESIGN_BIN}" --force --sign - "${file}")
        set(infoCmd "${CODESIGN_BIN}" --verify --strict --verbose=2 "${file}")
    elseif(RCODESIGN_BIN)
        set(signCmd "${RCODESIGN_BIN}" sign "${file}")
        set(infoCmd "${RCODESIGN_BIN}" print-signature-info "${file}")
    else()
        set(signCmd "${LDID_BIN}" -S "${file}")
        set(infoCmd "${LDID_BIN}" -h "${file}")
    endif()

    execute_process(COMMAND ${signCmd} RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Signing ${relFile} failed:\n${out}")
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
    message(NOTICE "Signed: ${relFile} (${info})")
endforeach()
