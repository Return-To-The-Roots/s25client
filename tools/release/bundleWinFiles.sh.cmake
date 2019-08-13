#!/bin/bash

set -euo pipefail

CMAKE_COMMAND=@CMAKE_COMMAND@
SYSTEM_NAME=@CMAKE_SYSTEM_NAME@
SYSTEM_ARCH=@PLATFORM_ARCH@
SYSROOT=@CMAKE_FIND_ROOT_PATH@
CXX=@CMAKE_CXX_COMPILER@
OBJDUMP=@CMAKE_OBJDUMP@
RTTR_BINDIR=@RTTR_BINDIR@
RTTR_EXTRA_BINDIR=@RTTR_EXTRA_BINDIR@
RTTR_DRIVERDIR=@RTTR_DRIVERDIR@
RTTR_SRCDIR=@RTTR_SRCDIR@

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

CXX_SYSROOT=$(dirname $($CXX -v 2>&1 | grep COLLECT_LTO_WRAPPER | cut -d '=' -f 2))

if [ -z "${RTTR_SRCDIR}" ] ; then
  error_and_exit "RTTR_SRCDIR was not set"
fi

if [ "$SYSTEM_NAME" != "Windows" ]; then
  error_and_exit "Only for Windows"
fi

if [ -z "$(type -p $OBJDUMP)" ] ; then
  error_and_exit "You have to install objdump"
fi

echo "## Installing for \"${SYSTEM_NAME}\""
echo "## Using Binary Dir \"${RTTR_BINDIR}\", \"${RTTR_EXTRA_BINDIR}\""
echo "## Using Driver Dir \"${RTTR_DRIVERDIR}\""

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

case "$SYSTEM_ARCH" in
  i686|*86)
    arch_dir=win32
  ;;
  x86_64|*64)
    arch_dir=win64
  ;;
esac

# exclude system dlls
DLL_BLACKLIST=(
  GDI32.dll KERNEL32.dll msvcrt.dll OPENGL32.dll USER32.dll	ADVAPI32.dll
  IPHLPAPI.DLL ole32.dll SHELL32.dll WS2_32.dll WINMM.DLL CRYPT32.dll
  wldap32.dll IMM32.DLL OLEAUT32.dll VERSION.dll
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
    FOUND=0
    for P in "$SYSROOT" "$CXX_SYSROOT" "${RTTR_SRCDIR}/external/lua/${arch_dir}" ; do
      for SUFFIX in "" lib/ bin/ ; do
        local candidate="$P/$SUFFIX$dep_dll"
        if [ -f "${candidate}" ] ; then
          echo "Found $(basename $candidate)"
          cp "${candidate}" "$to"
          copy_dependend_dlls "$to" "${candidate}"
          FOUND=1
          break
        fi
      done
      if [ $FOUND -eq 1 ] ; then
        break
      fi
    done
    if [ $FOUND -eq 0 ] ; then
      error_and_exit "Unable to find $dep_dll for $binary"
    fi
  done
}

binaries=()
binaries+=( "${RTTR_BINDIR}/"{s25client,s25edit}.exe )
binaries+=( "${RTTR_EXTRA_BINDIR}/"s25update.exe )
binaries+=( "${RTTR_DRIVERDIR}/"{video/libvideoWinAPI,video/libvideoSDL,video/libvideoSDL2,audio/libaudioSDL}.dll )

for binary in "${binaries[@]}" ; do
  to=$(dirname $binary)
  if [[ "$to" = "${RTTR_DRIVERDIR}"/* ]] ; then
    to="$RTTR_BINDIR"
  fi
  copy_dependend_dlls "${DESTDIR}$to" "$binary"
  echo ""
done
