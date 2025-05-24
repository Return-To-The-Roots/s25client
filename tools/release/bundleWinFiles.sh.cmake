#!/bin/bash

# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

CMAKE_COMMAND="@CMAKE_COMMAND@"
SYSTEM_NAME="@CMAKE_SYSTEM_NAME@"
SYSROOT="@CMAKE_FIND_ROOT_PATH@"
CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES="@CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES@"
CXX="@CMAKE_CXX_COMPILER@"
OBJDUMP="@CMAKE_OBJDUMP@"
RTTR_BINDIR="@RTTR_BINDIR@"

###############################################################################

mecho()
{
  COLOR=$1
  shift
  $CMAKE_COMMAND -E cmake_echo_color --bold $COLOR "$*"
}

error_and_exit()
{
  mecho --red "$*" >&2
  exit 1
}

if [ "$SYSTEM_NAME" != "Windows" ]; then
  error_and_exit "Only for Windows"
fi

if [ ! -f "$OBJDUMP" ] ; then
  error_and_exit "You have to install objdump"
fi

echo "## Using Binary Dir \"${RTTR_BINDIR}\""

###############################################################################

DESTDIR="${DESTDIR:-}"

# strip ending slash from $DESTDIR
DESTDIR=${DESTDIR%/}
[ -n "$CMAKE_INSTALL_PREFIX" ] && CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX%/}

# adding the slash again if DESTDIR is not empty
if [ -n "$DESTDIR" ] ; then
  DESTDIR=${DESTDIR}/
  mecho --green "## Using Destination Dir \"${DESTDIR}\""
fi

if [ -n "$CMAKE_INSTALL_PREFIX" ] ; then
  DESTDIR=${DESTDIR}${CMAKE_INSTALL_PREFIX}/
  mecho --green "## Updating Destination Dir with CMake-Install-Prefix \"${DESTDIR}\""
fi

###############################################################################

mecho --blue "## Copying dependencies"

SEARCH_DIRS=(${SYSROOT//;/ } ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES//;/ })

# exclude system dlls
DLL_BLACKLIST=(
  GDI32.dll KERNEL32.dll msvcrt.dll OPENGL32.dll USER32.dll	ADVAPI32.dll
  IPHLPAPI.DLL ole32.dll SHELL32.dll WS2_32.dll WINMM.DLL CRYPT32.dll
  wldap32.dll IMM32.DLL OLEAUT32.dll VERSION.dll api-ms-win-core-synch-l1-2-0.dll
  SETUPAPI.DLL
)
is_blacklisted_dll()
{
  trap "$(shopt -p nocasematch)" RETURN
  shopt -s nocasematch
  local dll="${1}"
  for blacklisted in "${DLL_BLACKLIST[@]}"; do
    if [[ "$dll" == "${blacklisted}" ]]; then
      return 0
    fi
  done
  return 1
}

resolve_dll()
{
  local dll="$1"
  for DIR in "${SEARCH_DIRS[@]}" ; do
    for SUFFIX in "" lib bin ; do
      local candidate="$DIR/$SUFFIX/$dll"
      if [ -f "${candidate}" ] ; then
        echo "$candidate"
        return 0
      fi
    done
  done
  return 1
}

copy_dependend_dlls()
{
  local to="$1"
  local binary="$2"

  echo "Searching for dependencies of $binary to copy to $to"
  dep_dlls="$($OBJDUMP -p $binary | grep "DLL Name" | cut -f 3 -d " ")"
  for dep_dll in $dep_dlls ; do
    if is_blacklisted_dll "$dep_dll"; then
      continue
    fi
    if [ -f "$to/$dep_dll" ]; then
      continue
    fi
    local resolved_dll=$(resolve_dll "$dep_dll" || true)
    if [ -n "$resolved_dll" ] ; then
      echo "Found $(basename $resolved_dll)"
      cp "${resolved_dll}" "$to"
      copy_dependend_dlls "$to" "${resolved_dll}"
    else
      error_and_exit "Unable to find $dep_dll for $binary"
    fi
  done
}

binaries=( $(find "${DESTDIR}" -name '*.exe') )
plugins=( $(find "${DESTDIR}" -name '*.dll') libvorbisfile-3.dll )

for binary in "${binaries[@]}" ; do
  to=$(dirname "$binary")
  copy_dependend_dlls "$to" "$binary"
  echo ""
done

MAIN_FOLDER="${DESTDIR}$RTTR_BINDIR"

for plugin in "${plugins[@]}" ; do
  if [[ ! "$plugin" =~ ^/ ]] ; then
    plugin=$(resolve_dll "$plugin")
    cp "$plugin" "$MAIN_FOLDER"
  fi
  copy_dependend_dlls "$MAIN_FOLDER" "$plugin"
  echo ""
done
