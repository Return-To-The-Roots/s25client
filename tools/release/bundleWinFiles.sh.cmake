#!/bin/bash

set -euo pipefail

# Editable Variables
CMAKE_COMMAND=@CMAKE_COMMAND@

###############################################################################

if [ -z "$(type -p $CMAKE_COMMAND)" ] ; then
	echo "You have to install CMake" >&2
	exit 1
fi

###############################################################################

mecho()
{
	COLOR=$1
	shift
	$CMAKE_COMMAND -E cmake_echo_color --bold $COLOR "$*"
}

###############################################################################

SYSTEM_NAME=@CMAKE_SYSTEM_NAME@
SYSTEM_ARCH=@PLATFORM_ARCH@
IS_CROSS_COMPILE=@CMAKE_CROSSCOMPILING@
SYSROOT=@CMAKE_FIND_ROOT_PATH@
CXX=@CMAKE_CXX_COMPILER@
OBJCOPY=@CMAKE_OBJCOPY@
OBJDUMP=@CMAKE_OBJDUMP@
STRIP=@CMAKE_STRIP@
RTTR_BINDIR=@RTTR_BINDIR@
RTTR_EXTRA_BINDIR=@RTTR_EXTRA_BINDIR@
RTTR_DATADIR=@RTTR_DATADIR@
RTTR_LIBDIR=@RTTR_LIBDIR@
RTTR_DRIVERDIR=@RTTR_DRIVERDIR@

RTTR_SRCDIR=@RTTR_SRCDIR@

if [ -z "${RTTR_SRCDIR}" ] ; then
	echo "RTTR_SRCDIR was not set" >&2
	exit 1
fi

if [ "$SYSTEM_NAME" != "Darwin" ] && [ -z "$(type -p $OBJCOPY)" ] ; then
	echo "You have to install objcopy" >&2
	exit 1
fi

if [ "$SYSTEM_NAME" == "Windows" ] && [ -z "$(type -p $OBJDUMP)" ] ; then
	echo "You have to install objdump" >&2
	exit 1
fi

if [ -z "$(type -p $STRIP)" ] ; then
	echo "You have to install strip" >&2
	exit 1
fi

echo "## Installing for \"${SYSTEM_NAME}\""
echo "## Using Binary Dir \"${RTTR_BINDIR}\", \"${RTTR_EXTRA_BINDIR}\""
echo "## Using Data Dir \"${RTTR_DATADIR}\""
echo "## Using Library Dir \"${RTTR_LIBDIR}\""

###############################################################################

DESTDIR="${DESTDIR:-}"

# strip ending slash from $DESTDIR
DESTDIR=${DESTDIR%/}
[ -n "$CMAKE_INSTALL_PREFIX" ] && CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX%/}

# adding the slash again if DESTDIR is not empty
if [ -n "$DESTDIR" ] ; then
	DESTDIR=${DESTDIR}/
	mecho --red "## Using Destination Dir \"${DESTDIR}\""
fi

if [ -n "$CMAKE_INSTALL_PREFIX" ] ; then
	DESTDIR=${DESTDIR}${CMAKE_INSTALL_PREFIX}/
	mecho --red "## Updating Destination Dir with CMake-Install-Prefix \"${DESTDIR}\""
fi

###############################################################################

mecho --blue "## Removing files which are unused (but installed by cmake)"
rm -vf ${DESTDIR}${RTTR_DRIVERDIR}/video/libvideo*.{a,lib}
rm -vf ${DESTDIR}${RTTR_DRIVERDIR}/audio/libaudio*.{a,lib}

extract_debug_symbols()
{
	local FILE=$1

	if [ "$SYSTEM_NAME" == "Darwin" ]; then
		# Can't extract symbols for apple, so just strip them
		$STRIP -S ${DESTDIR}s25client.app/$FILE
		return 0
	fi

	pushd ${DESTDIR}
	#mkdir -vp dbg/$(dirname $FILE)
	#echo "$OBJCOPY --only-keep-debug $FILE dbg/$FILE.dbg"
	#$OBJCOPY --only-keep-debug $FILE dbg/$FILE.dbg
	echo "$OBJCOPY --strip-debug $FILE"
	$OBJCOPY --strip-debug $FILE
	#echo "$OBJCOPY --add-gnu-debuglink=dbg/$FILE.dbg $FILE"
	#$OBJCOPY --add-gnu-debuglink=dbg/$FILE.dbg $FILE
	popd
}

mecho --blue "## Extracting debug info from files and saving them into dbg"

binaries=()
case "$SYSTEM_NAME" in
	Darwin)
		echo "extraction of debug symbols for Apple currently not supported" >&2
		exe_suffix=""
		lib_suffix=".dylib"
	;;
	Linux|FreeBSD)
		exe_suffix=""
		lib_suffix=".so"
	;;
	Windows)
		exe_suffix=".exe"
		lib_suffix=".dll"
		binaries+=( "${RTTR_DRIVERDIR}/video/libvideoWinAPI.dll" )
	;;
	*)
		echo "$SYSTEM_NAME not supported" >&2
		exit 1
	;;
esac

binaries+=( "${RTTR_BINDIR}/"{s25client,s25edit}${exe_suffix} )
binaries+=( "${RTTR_EXTRA_BINDIR}/"s25update${exe_suffix} )
binaries+=( "${RTTR_DRIVERDIR}/"{video/libvideoSDL,video/libvideoSDL2,audio/libaudioSDL}${lib_suffix} )

# strip out debug symbols into external file
for binary in "${binaries[@]}" ; do
	extract_debug_symbols $binary
done

mecho --blue "## Performing additional tasks"

case "$SYSTEM_NAME" in
	Darwin)
		# All done in CMake
	;;
	Windows)
		lua=""
		case "$SYSTEM_ARCH" in
			i686|*86)
				lua=win32
			;;
			x86_64|*64)
				lua=win64
			;;
		esac

		# exclude system dlls
		DLL_BLACKLIST="GDI32.dll KERNEL32.dll msvcrt.dll OPENGL32.dll USER32.dll ADVAPI32.dll IPHLPAPI.DLL ole32.dll SHELL32.dll WS2_32.dll WINMM.DLL CRYPT32.dll wldap32.dll IMM32.DLL OLEAUT32.dll VERSION.dll"

		copy_dll()
		{
			local dll=$1
			local to=$2

			echo -n "$(basename $dll) "
			cp $dll ${DESTDIR}$to || return 1
			return 0
		}

		copy_dlls()
		{
			local to=$1
			shift
			local dlls=$*

			for dll in "${dlls[@]}" ; do
				dep_dlls="$($OBJDUMP -p $dll | grep "DLL Name" | cut -f 3 -d " " | egrep -i -v "${DLL_BLACKLIST// /|}")"
				for dep_dll in $dep_dlls ; do
					FOUND=0
					CXX_SYSROOT=$(dirname $($CXX -v 2>&1 | grep COLLECT_LTO_WRAPPER | cut -d '=' -f 2))
					for P in $SYSROOT $CXX_SYSROOT ${RTTR_SRCDIR}/external/lua/${lua} ; do
						for SUFFIX in "" lib/ bin/ ; do
							if [ -f $P/$SUFFIX$dep_dll ] ; then
								copy_dll $P/$SUFFIX$dep_dll $to || return 1
								copy_dlls $to $P/$SUFFIX$dep_dll || return 1
								FOUND=1
								break
							fi
						done
					done
					if [ $FOUND -eq 0 ] ; then
						echo "Unable to find $dep_dll for $dll" >&2
						return 1
					fi
				done
			done
			return 0
		}

		for binary in "${binaries[@]}" ; do
			to=$(dirname $binary)
			if [[ "$to" = ${RTTR_DRIVERDIR}/* ]] ; then
				to=$RTTR_BINDIR
			fi
			echo "Copying dependency dlls of $binary to ${DESTDIR}$to"
			copy_dlls $to $binary || exit 1
			echo ""
		done

		if [ -d ${DESTDIR}S2 ]; then
			rmdir --ignore-fail-on-non-empty -v ${DESTDIR}S2
		fi
	;;
	Linux)
		miniupnpc=/usr/lib/libminiupnpc.so
		case "$SYSTEM_ARCH" in
			i686|*86)
				if [ "${IS_CROSS_COMPILE}" = "TRUE" ] ; then
					miniupnpc=/usr/i686-pc-linux-gnu/lib/libminiupnpc.so
				elif [ ! -f $miniupnpc ]; then
					# Use fallback
					miniupnpc=/usr/lib/i686-linux-gnu/libminiupnpc.so
				fi
			;;
			x86_64|*64)
				if [ "${IS_CROSS_COMPILE}" = "TRUE" ] ; then
					miniupnpc=/usr/x86_64-pc-linux-gnu/lib/libminiupnpc.so
				elif [ ! -f $miniupnpc ]; then
					# Use fallback
					miniupnpc=/usr/lib/x86_64-linux-gnu/libminiupnpc.so
				fi
			;;
		esac

		if [ -f $miniupnpc ] ; then
			mkdir -p ${DESTDIR}/lib/ || exit 1
			cp -rv $miniupnpc* ${DESTDIR}/lib/ || exit 1
		else
			echo "libminiupnpc.so not found at $miniupnpc" >&2
			echo "will not bundle it in your installation" >&2
			echo "install it via \"sudo apt-get install miniupnpc\"" >&2
		fi
	;;
	FreeBSD)
		miniupnpc=/usr/local/lib/libminiupnpc.so
		if [ -f $miniupnpc ] ; then
			mkdir -p ${DESTDIR}/lib/ || exit 1
			cp -rv ${miniupnpc}* ${DESTDIR}/lib/ || exit 1
		else
			echo "libminiupnpc.so not found at $miniupnpc" >&2
			echo "will not bundle it in your installation" >&2
			echo "install it via \"sudo pkg install miniupnpc\"" >&2
		fi
	;;
	*)
		echo "$SYSTEM_NAME not supported" >&2
		exit 1
	;;
esac

exit 0

###############################################################################
