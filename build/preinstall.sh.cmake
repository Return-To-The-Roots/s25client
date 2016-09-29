#!/bin/bash

# Editable Variables
CMAKE_COMMAND=cmake

###############################################################################

if [ -z "$(type -p $CMAKE_COMMAND)" ] ; then
	echo "You have to install CMake" >&2
	exit 1
fi

if [ -z "@RTTR_SRCDIR@" ] ; then
	echo "RTTR_SRCDIR was not set by cmake" >&2
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
RTTR_PREFIX=@RTTR_PREFIX@
RTTR_BINDIR=@RTTR_BINDIR@
RTTR_DATADIR=@RTTR_DATADIR@
RTTR_LIBDIR=@RTTR_LIBDIR@
RTTR_DRIVERDIR=@RTTR_DRIVERDIR@

###############################################################################

echo "## Installing for \"${SYSTEM_NAME}\""
echo "## Using Path-Prefix \"${RTTR_PREFIX}\""
echo "## Using Binary Dir \"${RTTR_BINDIR}\""
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

mecho --blue "## Creating directories"
mkdir -vp ${DESTDIR}${RTTR_BINDIR} || exit 1
mkdir -vp ${DESTDIR}${RTTR_DATADIR} || exit 1
mkdir -vp ${DESTDIR}${RTTR_DATADIR}/S2 || exit 1
mkdir -vp ${DESTDIR}${RTTR_DATADIR}/RTTR || exit 1
mkdir -vp ${DESTDIR}${RTTR_LIBDIR} || exit 1
mkdir -vp ${DESTDIR}${RTTR_DRIVERDIR}/video || exit 1
mkdir -vp ${DESTDIR}${RTTR_DRIVERDIR}/audio || exit 1
mkdir -vp ${DESTDIR}${RTTR_DATADIR}/../doc/s25rttr || exit 1

mecho --blue "## Installing binaries"

case "$SYSTEM_NAME" in
	Windows)
		cp -v ${RTTR_SRCDIR}/release/bin/rttr.bat ${DESTDIR}${RTTR_BINDIR} || exit 1
	;;
	Linux|FreeBSD)
		cp -v ${RTTR_SRCDIR}/release/bin/rttr.sh ${DESTDIR}${RTTR_BINDIR} || exit 1
	;;
	Darwin)
	;;
	*)
		echo "$SYSTEM_NAME not supported" >&2
		exit 1
	;;
esac

mecho --blue "## Installing RTTR directory"
if [ -d ${RTTR_SRCDIR}/RTTR/.svn ] ; then
	LANG=C svn --force --non-interactive export ${RTTR_SRCDIR}/RTTR ${DESTDIR}${RTTR_DATADIR}/RTTR || exit 1
elif [ -d ${RTTR_SRCDIR}/RTTR/.bzr ] ; then
	LANG=C bzr export ${RTTR_SRCDIR}/RTTR ${DESTDIR}${RTTR_DATADIR}/RTTR || exit 1
else
	cp -rv ${RTTR_SRCDIR}/RTTR/* ${DESTDIR}${RTTR_DATADIR}/RTTR || exit 1
fi
rm -f ${DESTDIR}${RTTR_DATADIR}/RTTR/languages/*.po

mecho --blue "## Installing language files"
cp -v ${RTTR_SRCDIR}/RTTR/languages/*.mo ${DESTDIR}${RTTR_DATADIR}/RTTR/languages/ || exit 1

mecho --blue "## Installing additional documents"
case "$SYSTEM_NAME" in
	Windows)
		cp -v ${RTTR_SRCDIR}/RTTR/texte/readme.txt ${DESTDIR} || exit 1
		cp -v ${RTTR_SRCDIR}/RTTR/texte/keyboardlayout.txt ${DESTDIR} || exit 1
	;;
	*)
		cp -v ${RTTR_SRCDIR}/RTTR/texte/readme.txt ${DESTDIR}${RTTR_DATADIR}/../doc/s25rttr || exit 1
		cp -v ${RTTR_SRCDIR}/RTTR/texte/keyboardlayout.txt ${DESTDIR}${RTTR_DATADIR}/../doc/s25rttr || exit 1
	;;
esac

mecho --blue "## Installing S2 placeholder"

case "$SYSTEM_NAME" in
	Windows)
		echo "creating ${DESTDIR}put\ your\ S2-Installation\ in\ here"
		echo "put your S2-Installation in here" > ${DESTDIR}put\ your\ S2-Installation\ in\ here || exit 1
	;;
	*)
		echo "creating ${DESTDIR}${RTTR_DATADIR}/S2/put\ your\ S2-Installation\ in\ here"
		echo "put your S2-Installation in here" > ${DESTDIR}${RTTR_DATADIR}/S2/put\ your\ S2-Installation\ in\ here || exit 1
	;;
esac

exit 0

###############################################################################
