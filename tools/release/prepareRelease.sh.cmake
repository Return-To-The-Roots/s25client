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
OBJCOPY=@CMAKE_OBJCOPY@
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

if [ -z "$(type -p $STRIP)" ] ; then
	echo "You have to install strip" >&2
	exit 1
fi

echo "## Installing for \"${SYSTEM_NAME}\""
echo "## Using Binary Dir \"${RTTR_BINDIR}\", \"${RTTR_EXTRA_BINDIR}\""
echo "## Using Data Dir \"${RTTR_DATADIR}\""
echo "## Using Library Dir \"${RTTR_LIBDIR}\""

###############################################################################

# strip ending slash from $DESTDIR
DESTDIR=${DESTDIR%/}

# adding the slash again if DESTDIR is not empty
if [ ! -z "$DESTDIR" ] ; then
	DESTDIR=${DESTDIR}/
	mecho --red "## Using Destination Dir \"${DESTDIR}\""
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
		$STRIP -S ${DESTDIR}$FILE
		return 0
	fi

	pushd ${DESTDIR}
	mkdir -vp dbg/$(dirname $FILE)
	echo "$OBJCOPY --only-keep-debug $FILE dbg/$FILE.dbg"
	$OBJCOPY --only-keep-debug $FILE dbg/$FILE.dbg
	echo "$OBJCOPY --strip-debug $FILE"
	$OBJCOPY --strip-debug $FILE
	echo "$OBJCOPY --add-gnu-debuglink=dbg/$FILE.dbg $FILE"
	$OBJCOPY --add-gnu-debuglink=dbg/$FILE.dbg $FILE
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
binaries+=( "${RTTR_EXTRA_BINDIR}/"{s25update,sound-convert,s-c_resample}${exe_suffix} )
binaries+=( "${RTTR_DRIVERDIR}/"{video/libvideoSDL,audio/libaudioSDL}${lib_suffix} )

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

		cp -v ${RTTR_SRCDIR}/external/lua/${lua}/lua52.dll ${DESTDIR} || exit 1

		if [ -f $SYSROOT/bin/libgcc_s_sjlj-1.dll ] ; then
			cp -v $SYSROOT/bin/libgcc_s_sjlj-1.dll ${DESTDIR} || exit 1
			cp -v $SYSROOT/bin/libgcc_s_sjlj-1.dll ${DESTDIR}RTTR || exit 1
		else
			cp -v $SYSROOT/bin/libgcc_s_seh-1.dll ${DESTDIR} || exit 1
			cp -v $SYSROOT/bin/libgcc_s_seh-1.dll ${DESTDIR}RTTR || exit 1
		fi
		
		if [ -f $SYSROOT/bin/libintl-8.dll ] ; then
			cp -v $SYSROOT/bin/libintl-8.dll ${DESTDIR} || exit 1
		fi
		
		if [ -f $SYSROOT/bin/libminiupnpc-5.dll ] ; then
			cp -v $SYSROOT/bin/zlib1.dll ${DESTDIR}RTTR || exit 1
			cp -v $SYSROOT/bin/libminiupnpc-5.dll ${DESTDIR} || exit 1
		else
			cp -v $SYSROOT/bin/libminiupnpc.dll ${DESTDIR} || exit 1
		fi
		
		if [ -f $SYSROOT/bin/libstdc++-6.dll ] ; then
			cp -v $SYSROOT/bin/libstdc++-6.dll ${DESTDIR} || exit
			cp -v $SYSROOT/bin/libstdc++-6.dll ${DESTDIR}RTTR || exit
		fi

		cp -v $SYSROOT/bin/libiconv-2.dll ${DESTDIR} || exit 1

		cp -v $SYSROOT/bin/SDL.dll ${DESTDIR} || exit 1
		cp -v $SYSROOT/bin/SDL_mixer.dll ${DESTDIR} || exit 1
		cp -v $SYSROOT/bin/libogg-0.dll ${DESTDIR} || exit 1
		cp -v $SYSROOT/bin/libvorbis-0.dll ${DESTDIR} || exit 1
		cp -v $SYSROOT/bin/libvorbisfile-3.dll ${DESTDIR} || exit 1

		cp -v $SYSROOT/bin/libcurl-4.dll ${DESTDIR}RTTR || exit 1

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
