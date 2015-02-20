#!/bin/bash
###############################################################################
## $Id: cmake.sh 7687 2011-12-30 09:17:47Z FloSoft $
###############################################################################

# Editable Variables
CMAKE_COMMAND=cmake

###############################################################################

if [ -z "$(type -p $CMAKE_COMMAND)" ] ; then
	echo "You have to install CMake"
	exit 1
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

if [ ! -e bin ] ; then
	mecho --blue "Creating symlink «bin» ..."
	ln -vs . bin
fi

if [ ! -e RTTR ] ; then
	if [ -e "${SRCDIR}/RTTR" ] ; then
		mecho --blue "Creating symlink «RTTR» ..."
		ln -vs "${SRCDIR}/RTTR" RTTR
	else
		mecho --red "Directory «RTTR» missing!"
		exit 1
	fi
fi

if [ ! -e share ] ; then
	mecho --blue "Creating symlink «share» ..."
	ln -vs . share
fi

if [ ! -e s25rttr ] ; then
	mecho --blue "Creating symlink «s25rttr» ..."
	ln -vs . s25rttr
fi

if [ ! -e S2 ] ; then
	if [ -e "${SRCDIR}/S2" ] ; then
		mecho --blue "Creating symlink «S2» ..."
		ln -vs "${SRCDIR}/S2" S2
	else
		mecho --red "Directory «S2» missing!"
		mecho --yellow "Direct debugging from this directory will not work then!"
	fi
fi

if ( [ ! -f cleanup.sh ] && [ -f "${SRCDIR}/build/cleanup.sh" ] ) ; then
	mecho --blue "Creating symlink «cleanup.sh» ..."
	ln -vs $SRCDIR/build/cleanup.sh cleanup.sh
fi

if ( [ ! -f cmake.sh ] && [ -f "${SRCDIR}/build/cmake.sh" ] ) ; then
	mecho --blue "Creating symlink «cmake.sh» ..."
	ln -vs $SRCDIR/build/cmake.sh cmake.sh
fi

###############################################################################

PREFIX=/usr/local
BINDIR=
DATADIR=
LIBDIR=
ARCH=
NOARCH=
GENERATOR=
PARAMS=""
as_cr_letters='abcdefghijklmnopqrstuvwxyz'
as_cr_LETTERS='ABCDEFGHIJKLMNOPQRSTUVWXYZ'
as_cr_Letters=$as_cr_letters$as_cr_LETTERS
as_cr_digits='0123456789'
as_cr_alnum=$as_cr_Letters$as_cr_digits

while test $# != 0 ; do
	case $1 in
		--*=*)
			ac_option=`expr "X$1" : 'X\([^=]*\)='`
			ac_optarg=`expr "X$1" : 'X[^=]*=\(.*\)'`
			ac_shift=:
		;;
		*)
			ac_option=$1
			ac_optarg=yes
			ac_shift=shift
		;;
	esac

	case $ac_option in
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
		-arch | --arch | -target | --target)
			$ac_shift
			ARCH=$ac_optarg
		;;
		-no-arch | --no-arch)
			$ac_shift
			NOARCH="$NOARCH $ac_optarg"
		;;
		-generator | --generator)
			#$ac_shift
			GENERATOR="$ac_optarg"
		;;
		-enable-* | --enable-*)
			ac_feature=`expr "x$ac_option" : 'x-*enable-\([^=]*\)'`
			# Reject names that are not valid shell variable names.
			expr "x$ac_feature" : ".*[^-._$as_cr_alnum]" >/dev/null &&
			{
				echo "error: invalid feature name: $ac_feature" >&2
				{ (exit 1); exit 1; };
			}
			ac_feature=`echo $ac_feature | sed 's/[-.]/_/g'`
			if [ -z "$ac_optarg" ] ; then
				ac_optarg="yes"
			fi
			eval enable_$ac_feature=\$ac_optarg
		;;
		-disable-* | --disable-*)
		        ac_feature=`expr "x$ac_option" : 'x-*disable-\([^=]*\)'`
						# Reject names that are not valid shell variable names.
						expr "x$ac_feature" : ".*[^-._$as_cr_alnum]" >/dev/null &&
						{
								echo "error: invalid feature name: $ac_feature" >&2
								{ (exit 1); exit 1; };
						}
						ac_feature=`echo $ac_feature | sed 's/[-.]/_/g'`
						if [ -z "$ac_optarg" ] ; then
								ac_optarg="yes"
						fi
						eval disable_$ac_feature=\$ac_optarg
		;;
		-D*)
			PARAMS="$ac_option=$ac_optarg"
		;;
		*)
			echo "Unknown option: $ac_option"
			exit 1
		;;
	esac

	shift
done

if [ -z "$ARCH" ] ; then
	if [ "$(uname -s)" = "Darwin" ] ; then
		ARCH=apple.local
	elif [ "$(uname -s)" = "Linux" ] ; then
		ARCH=linux.local
	else
		ARCH=windows.local
	fi
fi

if [ -z "$GENERATOR" ] && [ "$(uname -s)" = "Darwin" ] ; then
	GENERATOR="Xcode"
fi

if [ -z "$BINDIR" ] ; then
	BINDIR=$PREFIX/bin
fi

if [ -z "$DATADIR" ] ; then
	DATADIR=$PREFIX/share/s25rttr
fi

if [ -z "$LIBDIR" ] ; then
	LIBDIR=$DATADIR
fi

###############################################################################

echo "Setting Path-Prefix to \"$PREFIX\""
PARAMS="$PARAMS -DPREFIX=$PREFIX"

echo "Setting Architecture to \"$ARCH\""
PARAMS="$PARAMS -DCOMPILEFOR_PLATFORM=$ARCH"

echo "Setting Binary Dir to \"$BINDIR\""
PARAMS="$PARAMS -DBINDIR=$BINDIR"

echo "Setting Data Dir to \"$DATADIR\""
PARAMS="$PARAMS -DDATADIR=$DATADIR"

echo "Setting Library Dir to \"$LIBDIR\""
PARAMS="$PARAMS -DLIBDIR=$LIBDIR"

echo "Disabling build of \"$NOARCH\""
if [ ! -z "$disable_arch" ] ; then
	NOARCH="$disable_arch $NOARCH"
fi
for I in $NOARCH ; do
	if [ ! -z "$I" ] ; then
		PARAMS="$PARAMS -DNO$I=$I"
	fi
done

case "$enable_debug" in
	yes|YES|Yes)
		mecho --magenta "Activating debug build"
		PARAMS="$PARAMS -DCMAKE_BUILD_TYPE=Debug"
	;;
	*)
		case "$enable_reldeb" in
			yes|YES|Yes)
				mecho --magenta "Activating release build with debug information"
				PARAMS="$PARAMS -DCMAKE_BUILD_TYPE=RelWithDebInfo"
			;;
			*)
				mecho --magenta "Activating release build"
				PARAMS="$PARAMS -DCMAKE_BUILD_TYPE=Release"
			;;
		esac
	;;
esac

case "$enable_verbose" in
	yes|YES|Yes)
		mecho --magenta "Activating verbose build"
		PARAMS="$PARAMS -DCMAKE_VERBOSE_MAKEFILE=On"
	;;
	*)
	;;
esac

###############################################################################

if [ ! -z "$GENERATOR" ] ; then
	echo "Generating files for \"$GENERATOR\""
else
	GENERATOR="Unix Makefiles"
fi

mecho --blue "Running \"cmake -G '$GENERATOR' -DCMAKE_INSTALL_PREFIX= ${PARAMS} '${SRCDIR}'\""
$CMAKE_COMMAND -G "$GENERATOR" -DCMAKE_INSTALL_PREFIX= $PARAMS "${SRCDIR}"

if [ $? != 0 ] ; then
	mecho --red "An error occured - please check above!"
	exit 1
fi

MAKE="make"
if [ "$(uname -s)" = "Darwin" ] ; then
	MAKE="xcodebuild"
fi

mecho --blue "Now type \"$MAKE\" to build project"

exit 0

###############################################################################
