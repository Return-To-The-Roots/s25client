#!/bin/bash

set -euxo pipefail

# Editable Variables
CMAKE_COMMAND=cmake

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
		i686-apple-darwin10-strip -S ${DESTDIR}$FILE
		return 0
	fi

	objcopyArch=""
	case "$SYSTEM_ARCH" in
		i686|*86)
			objcopyArch="i686"
		;;
		x86_64|*64)
			objcopyArch="x86_64"
		;;
		powerpc|ppc)
			objcopyArch="powerpc"
		;;
		*)
			echo "$SYSTEM_ARCH not supported" >&2
			return 1
		;;
	esac

	objcopyTarget=""
	case "$SYSTEM_NAME" in
		Windows)
			objcopyTarget="-pc-mingw32"
		;;
		Linux)
			objcopyTarget="-pc-linux-gnu"
		;;
		FreeBSD)
			objcopy="objcopy"  # no cross-build support for FreeBSD
		;;
		*)
			echo "$SYSTEM_NAME not supported" >&2
			return 1
		;;
	esac

	# Set if not yet set
	: ${objcopy:=${objcopyArch}${objcopyTarget}-objcopy}

	if ! `${objcopy} -V >/dev/null 2>&1`; then
		# Use fallback
		case "$SYSTEM_NAME" in
			Windows)
				objcopyTarget="-mingw32"
			;;
			Linux)
				objcopyTarget="-linux-gnu"
			;;
		esac
		objcopy="${objcopyArch}${objcopyTarget}-objcopy"
	fi

	pushd ${DESTDIR}
	mkdir -vp dbg/$(dirname $FILE)
	echo "${objcopy} --only-keep-debug $FILE dbg/$FILE.dbg"
	${objcopy} --only-keep-debug $FILE dbg/$FILE.dbg
	echo "${objcopy} --strip-debug $FILE"
	${objcopy} --strip-debug $FILE
	echo "${objcopy} --add-gnu-debuglink=dbg/$FILE.dbg $FILE"
	${objcopy} --add-gnu-debuglink=dbg/$FILE.dbg $FILE
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
		# create app-bundle for apple
		# app anlegen
		contentsPath="${DESTDIR}s25client.app/Contents"
		mkdir -vp ${contentsPath}/{MacOS,Resources} || exit 1
		macOSPath="${contentsPath}/MacOS"
		frameworksPath="${macOSPath}/Frameworks"

		# frameworks kopieren
		mkdir -vp ${frameworksPath} || exit 1
		mkdir -vp ${frameworksPath}/{SDL,SDL_mixer}.framework || exit 1

		srcFrameworksPath="/Library/Frameworks"
		if [ ! -d /Library/Frameworks ] ; then
			srcFrameworksPath="/usr/lib/apple/SDKs/Library/Frameworks"
		fi
		cp -r ${srcFrameworksPath}/SDL.framework ${frameworksPath} || exit 1
		cp -r ${srcFrameworksPath}/SDL_mixer.framework ${frameworksPath} || exit 1

		# remove headers and additional libraries from the frameworks
		find ${frameworksPath}/ -name Headers -exec rm -rf {} \; || true
		find ${frameworksPath}/ -name Resources -exec rm -rf {} \; || true

		SDK=/Developer/SDKs/MacOSX10.5.sdk
		if [ ! -d $SDK ] ; then
			SDK=/usr/lib/apple/SDKs/MacOSX10.5.sdk
		fi

		# copy libs
		for LIBNAME in miniupnpc.5 boost_system boost_filesystem boost_iostreams boost_thread boost_locale boost_program_options ; do
			LIB=/usr/lib/lib${LIBNAME}.dylib
			if [ -f $SDK$LIB ] ; then
				cp -rv $SDK$LIB ${macOSPath} || exit 1
			else
				echo "$LIB was not found in $SDK" >&2
				exit 1
			fi
		done

		mkdir -vp ${macOSPath}/bin || exit 1
		mkdir -vp ${macOSPath}/libexec || exit 1
		mkdir -vp ${macOSPath}/lib || exit 1

		# binaries und paketdaten kopieren
		cp -v ${RTTR_SRCDIR}/tools/release/bin/macos/rttr.command ${macOSPath}/ || exit 1
		cp -v ${RTTR_SRCDIR}/tools/release/bin/macos/rttr.terminal ${macOSPath}/ || exit 1
		cp -v ${RTTR_SRCDIR}/tools/release/bin/macos/icon.icns ${contentsPath}/Resources/ || exit 1
		cp -v ${RTTR_SRCDIR}/tools/release/bin/macos/PkgInfo ${contentsPath}/ || exit 1
		cp -v ${RTTR_SRCDIR}/tools/release/bin/macos/Info.plist ${contentsPath}/ || exit 1
		mv -v ${DESTDIR}bin/* ${macOSPath}/bin/ || exit 1
		mv -v ${DESTDIR}libexec/* ${macOSPath}/libexec/ || exit 1
		mv -v ${DESTDIR}lib/* ${macOSPath}/lib/ || exit 1
		
		chmod +x ${macOSPath}/rttr.command || exit 1
		chmod +x ${macOSPath}/bin/* || exit 1
		chmod +x ${macOSPath}/libexec/s25rttr/* || exit 1

		# remove dirs if empty
		rmdir ${DESTDIR}bin || true
		rmdir ${DESTDIR}lib || true
		rmdir ${DESTDIR}libexec || true

		# RTTR-Ordner kopieren
		mv -v ${DESTDIR}share ${macOSPath}/ || exit 1
	;;
	Windows)
		mingw=/usr
		lua=""
		case "$SYSTEM_ARCH" in
			i686|*86)
				if [ -d /usr/i686-pc-mingw32 ]; then
					mingw=/usr/i686-pc-mingw32
				else
					mingw=/usr/i686-mingw32
				fi
				lua=win32
			;;
			x86_64|*64)
				if [ -d /usr/i686-pc-mingw32 ]; then
					mingw=/usr/x86_64-pc-mingw32
				else
					mingw=/usr/x86_64-mingw32
				fi
				lua=win64
			;;
		esac
		
		cp -v ${RTTR_SRCDIR}/external/lua/${lua}/lua52.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libgcc_s_sjlj-1.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libminiupnpc-5.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libiconv-2.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libintl-8.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libogg-0.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/SDL_mixer.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/SDL.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libvorbis-0.dll ${DESTDIR} || exit 1
		cp -v ${mingw}/bin/libvorbisfile-3.dll ${DESTDIR} || exit 1
		
		cp -v ${mingw}/bin/libgcc_s_sjlj-1.dll ${DESTDIR}RTTR || exit 1
		cp -v ${mingw}/bin/libcurl-4.dll ${DESTDIR}RTTR || exit 1
		cp -v ${mingw}/bin/zlib1.dll ${DESTDIR}RTTR || exit 1

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
		echo "$SYSTEM_ARCH not supported" >&2
		exit 1
	;;
esac

exit 0

###############################################################################
