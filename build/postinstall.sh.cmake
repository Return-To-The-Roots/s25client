#!/bin/bash
###############################################################################
## $Id: postinstall.sh.cmake 8390 2012-10-04 16:44:24Z marcus $
###############################################################################

# Editable Variables
CMAKE_COMMAND=cmake

###############################################################################

if [ -z "$(type -p $CMAKE_COMMAND)" ] ; then
	echo "You have to install CMake"
	exit 1
fi

if [ ! -z "@SRCDIR@" ] ; then
	SRCDIR=@SRCDIR@
fi

if [ -z "$SRCDIR" ] ; then
	SRCDIR="$(dirname "$0")/.."
fi

###############################################################################

mecho()
{
	COLOR=$1
	shift
	$CMAKE_COMMAND -E cmake_echo_color --bold $COLOR "$*"
}

###############################################################################

COMPILEFOR=@COMPILEFOR@
COMPILEARCH=@COMPILEARCH@
PREFIX=@PREFIX@
BINDIR=@BINDIR@
DATADIR=@DATADIR@
LIBDIR=@LIBDIR@

###############################################################################

while test $# != 0 ; do
	case $1 in
	--*=*)
		ac_option=`expr "X$1" : 'X\([^=]*\)='`
		ac_optarg=`expr "X$1" : 'X[^=]*=\(.*\)'`
		ac_shift=:
	;;
	*)
		ac_option=$1
		ac_optarg=$2
		ac_shift=shift
	;;
	esac

	case $ac_option in
	-compilefor | --compilefor)
		$ac_shift
		COMPILEFOR=$ac_optarg
	;;
	-compilearch | --compilearch)
		$ac_shift
		COMPILEARCH=$ac_optarg
	;;
	-prefix | --prefix)
		$ac_shift
		PREFIX=$ac_optarg
	;;
	-bindir | --bindir)
		$ac_shift
		BINDIR=$ac_optarg
	;;
	-datadir | --datadir)
		$ac_shift
		DATADIR=$ac_optarg
	;;
	-libdir | --libdir)
		$ac_shift
		LIBDIR=$ac_optarg
	;;
	*)
		echo "Unknown option: $ac_option"
		exit 1
	;;
	esac

	shift
done

if [ -z "${COMPILEFOR}" ] ; then
	COMPILEFOR=$(uname -s | tr '[:upper:]' '[:lower:]')
fi

if [ -z "${COMPILEARCH}" ] ;then
	COMPILEARCH=$(uname -m)
fi

if [ -z "${PREFIX}" ] ; then
	PREFIX=/usr/local
fi

if [ -z "${BINDIR}" ] ; then
	BINDIR=${PREFIX}/bin
fi

if [ -z "${DATADIR}" ] ; then
	DATADIR=${PREFIX}/share/s25rttr
fi

if [ -z "${LIBDIR}" ] ; then
	LIBDIR=${DATADIR}
fi

echo "## Installing for \"${COMPILEFOR}\""
echo "## Using Path-Prefix \"${PREFIX}\""
echo "## Using Binary Dir \"${BINDIR}\""
echo "## Using Data Dir \"${DATADIR}\""
echo "## Using Library Dir \"${LIBDIR}\""

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
rm -vf ${DESTDIR}${LIBDIR}/driver/video/libvideo*.{a,lib}
rm -vf ${DESTDIR}${LIBDIR}/driver/audio/libaudio*.{a,lib}

extract_debug_symbols()
{
	local FILE=$1

	objcopy=""
	case "$COMPILEFOR" in
		windows)
			objcopy="${objcopy}-pc-mingw32"
		;;
		linux)
			objcopy="${objcopy}-pc-linux-gnu"
		;;
		apple)
			echo "not supported"
			return 1
		;;
	esac
	objcopy="${objcopy}-objcopy"

	case "$COMPILEARCH" in
		i686|*86)
			objcopy="i686${objcopy}"
		;;
		x86_64|*64)
			objcopy="x86_64${objcopy}"
		;;
		powerpc|ppc)
			objcopy="powerpc${objcopy}"
		;;
	esac

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

# strip out debug symbols into external file
if [ "$COMPILEFOR" = "apple" ] ; then
	echo "extraction not supported ???"
	i686-apple-darwin10-strip -S ${DESTDIR}bin/s25client
	i686-apple-darwin10-strip -S ${DESTDIR}share/s25rttr/driver/video/libvideoSDL.dylib
	i686-apple-darwin10-strip -S ${DESTDIR}share/s25rttr/driver/audio/libaudioSDL.dylib
	i686-apple-darwin10-strip -S ${DESTDIR}share/s25rttr/RTTR/s25update
	i686-apple-darwin10-strip -S ${DESTDIR}share/s25rttr/RTTR/sound-convert
	i686-apple-darwin10-strip -S ${DESTDIR}share/s25rttr/RTTR/s-c_resample
elif [ "$COMPILEFOR" = "windows" ] ; then
	extract_debug_symbols s25client.exe
	extract_debug_symbols driver/video/libvideoWinAPI.dll
	extract_debug_symbols driver/video/libvideoSDL.dll
	extract_debug_symbols driver/audio/libaudioSDL.dll
	extract_debug_symbols RTTR/s25update.exe
	extract_debug_symbols RTTR/sound-convert.exe
	extract_debug_symbols RTTR/s-c_resample.exe
elif [ "$COMPILEFOR" = "linux" ] ; then
	extract_debug_symbols bin/s25client
	extract_debug_symbols share/s25rttr/driver/video/libvideoSDL.so
	extract_debug_symbols share/s25rttr/driver/audio/libaudioSDL.so
	extract_debug_symbols share/s25rttr/RTTR/s25update
	extract_debug_symbols share/s25rttr/RTTR/sound-convert
	extract_debug_symbols share/s25rttr/RTTR/s-c_resample
fi

mecho --blue "## Performing additional tasks"

# create app-bundle for apple
if [ "$COMPILEFOR" = "apple" ] ; then
	# app anlegen
	mkdir -vp ${DESTDIR}s25client.app/Contents/{MacOS,Resources} || exit 1

	# frameworks kopieren
	mkdir -vp ${DESTDIR}s25client.app/Contents/MacOS/Frameworks || exit 1
	mkdir -vp ${DESTDIR}s25client.app/Contents/MacOS/Frameworks/{SDL,SDL_mixer}.framework || exit 1

	if [ -d /Library/Frameworks ] ; then
		cp -r /Library/Frameworks/SDL.framework ${DESTDIR}s25client.app/Contents/MacOS/Frameworks || exit 1
		cp -r /Library/Frameworks/SDL_mixer.framework ${DESTDIR}s25client.app/Contents/MacOS/Frameworks || exit 1
	else
		cp -r /usr/lib/apple/SDKs/Library/Frameworks/SDL.framework ${DESTDIR}s25client.app/Contents/MacOS/Frameworks || exit 1
		cp -r /usr/lib/apple/SDKs/Library/Frameworks/SDL_mixer.framework ${DESTDIR}s25client.app/Contents/MacOS/Frameworks || exit 1
	fi

	# remove headers and additional libraries from the frameworks
	find ${DESTDIR}s25client.app/Contents/MacOS/Frameworks/ -name Headers -exec rm -rf {} \;
	find ${DESTDIR}s25client.app/Contents/MacOS/Frameworks/ -name Resources -exec rm -rf {} \;

	# copy miniupnp
	if [ -f /Developer/SDKs/MacOSX10.5.sdk/usr/lib/libminiupnpc.5.dylib ] ; then
		cp -rv /Developer/SDKs/MacOSX10.5.sdk/usr/lib/libminiupnpc.5.dylib ${DESTDIR}s25client.app/Contents/MacOS || exit 1
	elif  [ -f /usr/lib/apple/SDKs/MacOSX10.5.sdk/usr/lib/libminiupnpc.5.dylib ] ; then
		cp -rv /usr/lib/apple/SDKs/MacOSX10.5.sdk/usr/lib/libminiupnpc.5.dylib ${DESTDIR}s25client.app/Contents/MacOS || exit 1
	else
		echo "libminiupnpc.5.dylib was not found"
		exit 1
	fi

	mkdir -vp ${DESTDIR}s25client.app/Contents/MacOS/bin || exit 1

	# binaries und paketdaten kopieren
	cp -v ${SRCDIR}/release/bin/macos/rttr.command ${DESTDIR}s25client.app/Contents/MacOS/ || exit 1
	cp -v ${SRCDIR}/release/bin/macos/rttr.terminal ${DESTDIR}s25client.app/Contents/MacOS/ || exit 1
	cp -v ${SRCDIR}/release/bin/macos/icon.icns ${DESTDIR}s25client.app/Contents/Resources/ || exit 1
	cp -v ${SRCDIR}/release/bin/macos/PkgInfo ${DESTDIR}s25client.app/Contents/ || exit 1
	cp -v ${SRCDIR}/release/bin/macos/Info.plist ${DESTDIR}s25client.app/Contents/ || exit 1
	mv -v ${DESTDIR}bin/* ${DESTDIR}s25client.app/Contents/MacOS/bin/ || exit 1

	# remove dirs if empty
	rmdir ${DESTDIR}bin
	rmdir ${DESTDIR}lib

	# RTTR-Ordner kopieren
	mv -v ${DESTDIR}share ${DESTDIR}s25client.app/Contents/MacOS/ || exit 1

elif [ "$COMPILEFOR" = "windows" ] ; then
	mingw=/usr
	lua=""
	case "$COMPILEARCH" in
		i686|*86)
			mingw=/usr/i686-pc-mingw32
			lua=win32
		;;
		x86_64|*64)
			mingw=/usr/x86_64-pc-mingw32
			lua=win64
		;;
	esac
	
	cp -v ${SRCDIR}/contrib/lua/${lua}/lua52.dll ${DESTDIR} || exit 1
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

	rmdir --ignore-fail-on-non-empty -v ${DESTDIR}S2
elif [ "$COMPILEFOR" = "linux" ] ; then
	miniupnpc=/usr/lib/libminiupnpc.so
	case "$COMPILEARCH" in
		i686|*86)
			if [ ! "$(uname -m | sed s/i686/i386/g)" = "$COMPILEARCH" ] ; then
				miniupnpc=/usr/i686-pc-linux-gnu/lib/libminiupnpc.so
			fi
		;;
		x86_64|*64)
			if [ ! "$(uname -m)" = "$COMPILEARCH" ] ; then
				miniupnpc=/usr/x86_64-pc-linux-gnu/lib/libminiupnpc.so
			fi
		;;
	esac

	if [ -f $miniupnpc ] ; then
		mkdir -p ${DESTDIR}${PREFIX}/lib/ || exit 1
		cp -rv $miniupnpc* ${DESTDIR}${PREFIX}/lib/ || exit 1
	else
		echo "libminiupnpc.so not found at $miniupnpc"
		echo "will not bundle it in your installation"
		echo "install it from http://packages.siedler25.org by yourself"
	fi
fi

exit 0

###############################################################################
