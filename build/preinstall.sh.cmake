#!/bin/bash

# Editable Variables
CMAKE_COMMAND=cmake

###############################################################################

if [ -z "$(type -p $CMAKE_COMMAND)" ] ; then
	echo "You have to install CMake" >&2
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

SYSTEM_NAME=@CMAKE_SYSTEM_NAME@
PREFIX=@PREFIX@
BINDIR=@BINDIR@
DATADIR=@DATADIR@
LIBDIR=@LIBDIR@

###############################################################################

echo "## Installing for \"${SYSTEM_NAME}\""
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

mecho --blue "## Creating directories"
mkdir -vp ${DESTDIR}${BINDIR} || exit 1
mkdir -vp ${DESTDIR}${DATADIR} || exit 1
mkdir -vp ${DESTDIR}${DATADIR}/S2 || exit 1
mkdir -vp ${DESTDIR}${DATADIR}/RTTR || exit 1
mkdir -vp ${DESTDIR}${LIBDIR} || exit 1
mkdir -vp ${DESTDIR}${LIBDIR}/driver/video || exit 1
mkdir -vp ${DESTDIR}${LIBDIR}/driver/audio || exit 1
mkdir -vp ${DESTDIR}${DATADIR}/../doc/s25rttr || exit 1

mecho --blue "## Installing binaries"

case "$SYSTEM_NAME" in
	Windows)
		cp -v ${SRCDIR}/release/bin/rttr.bat ${DESTDIR}${BINDIR} || exit 1
	;;
	Linux|FreeBSD)
		cp -v ${SRCDIR}/release/bin/rttr.sh ${DESTDIR}${BINDIR} || exit 1
	;;
	Darwin)
	;;
	*)
		echo "$SYSTEM_NAME not supported" >&2
		exit 1
	;;
esac

mecho --blue "## Installing RTTR directory"
if [ -d ${SRCDIR}/RTTR/.svn ] ; then
	LANG=C svn --force --non-interactive export ${SRCDIR}/RTTR ${DESTDIR}${DATADIR}/RTTR || exit 1
elif [ -d ${SRCDIR}/RTTR/.bzr ] ; then
	LANG=C bzr export ${SRCDIR}/RTTR ${DESTDIR}${DATADIR}/RTTR || exit 1
else
	cp -rv ${SRCDIR}/RTTR/* ${DESTDIR}${DATADIR}/RTTR || exit 1
fi
rm -f ${DESTDIR}${DATADIR}/RTTR/languages/*.po

mecho --blue "## Installing language files"
cp -v ${SRCDIR}/RTTR/languages/*.mo ${DESTDIR}${DATADIR}/RTTR/languages/ || exit 1

mecho --blue "## Installing additional documents"
case "$SYSTEM_NAME" in
	Windows)
		cp -v ${SRCDIR}/RTTR/texte/readme.txt ${DESTDIR} || exit 1
		cp -v ${SRCDIR}/RTTR/texte/keyboardlayout.txt ${DESTDIR} || exit 1
	;;
	*)
		cp -v ${SRCDIR}/RTTR/texte/readme.txt ${DESTDIR}${DATADIR}/../doc/s25rttr || exit 1
		cp -v ${SRCDIR}/RTTR/texte/keyboardlayout.txt ${DESTDIR}${DATADIR}/../doc/s25rttr || exit 1
	;;
esac

mecho --blue "## Installing S2 placeholder"

case "$SYSTEM_NAME" in
	Windows)
		echo "creating ${DESTDIR}put\ your\ S2-Installation\ in\ here"
		echo "put your S2-Installation in here" > ${DESTDIR}put\ your\ S2-Installation\ in\ here || exit 1
	;;
	*)
		echo "creating ${DESTDIR}${DATADIR}/S2/put\ your\ S2-Installation\ in\ here"
		echo "put your S2-Installation in here" > ${DESTDIR}${DATADIR}/S2/put\ your\ S2-Installation\ in\ here || exit 1
	;;
esac

exit 0

###############################################################################
