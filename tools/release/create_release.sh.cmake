#!/bin/bash

set -euxo pipefail

usage()
{
	echo "$0 [nightly|stable]"
	echo "set environment FORCE to 1 to force update of archive"
	exit 1
}

cleanup()
{
	echo -n ""
	#rm -rf /tmp/$$
}

error()
{
    if [ $# != 0 ]; then
        echo "ERROR: $1"
    fi
	cleanup
	exit 1
}

if [[ $# != 1 ]] ; then
	usage
fi

TYPE=$1

# Configured by CMake
SRCDIR=@CMAKE_SOURCE_DIR@
BUILD_DIR=@CMAKE_BINARY_DIR@
PLATFORM_NAME=@PLATFORM_NAME@
PLATFORM_ARCH=@PLATFORM_ARCH@
VERSION=@RTTR_VERSION@
REVISION=@RTTR_REVISION@

RELEASEDEF=$SRCDIR/tools/release/release.$TYPE.def
source $RELEASEDEF || error

[ -d "$TARGET" ] || error "$RELEASEDEF does not contain TARGET"

[[ -n "${PLATFORM_NAME}" ]] || error "PLATFORM_NAME not found"
[[ -n "${PLATFORM_ARCH}" ]] || error "PLATFORM_ARCH not found"

ARCH="${PLATFORM_NAME}.${PLATFORM_ARCH}"

# current and new package directory
ARCHDIR=$TARGET/$ARCH
ARCHNEWDIR=$TARGET/$ARCH.new

rm -rf $ARCHNEWDIR
mkdir -p $ARCHNEWDIR/packed
mkdir -p $ARCHNEWDIR/unpacked
mkdir -p $ARCHNEWDIR/updater

touch $ARCHNEWDIR/.writetest || error
rm -f $ARCHNEWDIR/.writetest

# redirect output to log AND stdout
npipe=/tmp/$$.log
trap "rm -f $npipe" EXIT
mknod $npipe p
tee <$npipe $ARCHNEWDIR/build.log &
exec 1>&-
exec 1>$npipe

echo "Building $TYPE for $ARCH in $SRCDIR"

cd "${BUILD_DIR}
make || error

# get savegame version
SAVEGAMEVERSION=$(grep "; // SaveGameVersion -- " $SRCDIR/libs/s25main/Savegame.cpp | cut -d " " -f 6 | cut -d \; -f 1)

echo "Current version is: $VERSION-$REVISION"
echo "Savegame version:   $SAVEGAMEVERSION"

unpackedPath=$ARCHNEWDIR/unpacked/s25rttr_$VERSION

rm -rf "${unpackedPath}"

# Install into this folder
cmake . -DCMAKE_INSTALL_PREFIX="${unpackedPath}" -DRTTR_VERSION="${VERSION}" -DRTTR_REVISION="${REVISION}" || error

make install || error
DESTDIR="${unpackedPath}" ./prepareRelease.sh
if [ ! $? = 0 ]; then
	echo "error: Could not prepare release (strip executables etc.)"
	error
fi

# do they differ?
CHANGED=1
if [ "${FORCE:-0}" = "1" ] ; then
	echo "FORCE is set - forcing update"
elif [ -d $ARCHDIR/unpacked/s25rttr_$VERSION ] ; then
	diff -qrN $ARCHDIR/unpacked/s25rttr_$VERSION $unpackedPath && CHANGED=0 || CHANGED=1
fi

FORMAT=".tar.bz2"
if [[ "$ARCH" =~ windows.* ]] ; then
	FORMAT=".zip"
fi

# create packed data and updater
if [ $CHANGED -eq 1 ] || [ ! -f $ARCHDIR/packed/s25rttr$FORMAT ] ; then
	echo "creating new archive"

	# remove old build artefacts
	rm -f ../s25rttr*$FORMAT

	# pack
	case "$FORMAT" in
		.tar.bz2)
			tar -C $ARCHNEWDIR/unpacked \
				--exclude=.svn \
				--exclude=dbg \
				--exclude s25rttr_$VERSION/share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG \
				--exclude s25rttr_$VERSION/RTTR/MUSIC/SNG/SNG_*.OGG \
				--exclude s25rttr_$VERSION/s25client.app/Contents/MacOS/share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG \
				-cvjf $ARCHNEWDIR/packed/s25rttr$FORMAT s25rttr_$VERSION || error
		;;
		.zip)
			(cd $ARCHNEWDIR/unpacked && \
				zip -r9 \
					$ARCHNEWDIR/packed/s25rttr$FORMAT \
					-x "s25rttr_$VERSION/dbg/*" \
					-x "s25rttr_$VERSION/RTTR/MUSIC/SNG/SNG_*.OGG" \
					-- s25rttr_$VERSION) || error
		;;
	esac
	
	cp -v $ARCHNEWDIR/packed/s25rttr$FORMAT ../s25rttr_$VERSION-${REVISION}_$ARCH$FORMAT || exit

	if [ -d $ARCHNEWDIR/unpacked/s25rttr_$VERSION/dbg ] ; then
		case "$FORMAT" in
			.tar.bz2)
				tar -C $ARCHNEWDIR/unpacked \
					-cvjf $ARCHNEWDIR/packed/s25rttr_dbg$FORMAT s25rttr_$VERSION/dbg || error
			;;
			.zip)
				(cd $ARCHNEWDIR/unpacked && \
					zip -r9 \
						$ARCHNEWDIR/packed/s25rttr_dbg$FORMAT s25rttr_$VERSION/dbg) || error
			;;
		esac

		cp -v $ARCHNEWDIR/packed/s25rttr_dbg$FORMAT ../s25rttr-dbg_$VERSION-${REVISION}_$ARCH$FORMAT || exit 1 
	else
		touch ../s25rttr-dbg_$VERSION-${REVISION}_$ARCH$FORMAT
	fi
	
	# link to archive
	mkdir -p $ARCHIVE
	ln -v $ARCHNEWDIR/packed/s25rttr$FORMAT $ARCHIVE/s25rttr_$VERSION-${REVISION}_$ARCH$FORMAT || \
		cp -v $ARCHNEWDIR/packed/s25rttr$FORMAT $ARCHIVE/s25rttr_$VERSION-${REVISION}_$ARCH$FORMAT || exit 1

	if [ -d $ARCHNEWDIR/unpacked/s25rttr_$VERSION/dbg ] ; then
		ln -v $ARCHNEWDIR/packed/s25rttr_dbg$FORMAT $ARCHIVE/s25rttr-dbg_$VERSION-${REVISION}_$ARCH$FORMAT || \
			cp -v $ARCHNEWDIR/packed/s25rttr_dbg$FORMAT $ARCHIVE/s25rttr-dbg_$VERSION-${REVISION}_$ARCH$FORMAT || exit 1
	fi

	# do upload
    UPLOADTARGET="${UPLOADTARGET:-}"
	if [ ! "${NOUPLOAD:-0}" = "1" ] && [ ! -z "$UPLOADTARGET" ] ; then
        UPLOADTO="${UPLOADTO:-$VERSION/}"
		
		echo "uploading file to $UPLOADTARGET$UPLOADTO"
		ssh $UPLOADHOST "mkdir -vp $UPLOADPATH$UPLOADTO" || echo "mkdir $UPLOADPATH$UPLOADTO failed"
		rsync -avz --progress $ARCHIVE/s25rttr_$VERSION-${REVISION}_$ARCH$FORMAT $UPLOADTARGET$UPLOADTO || echo "scp failed"
		if [ ! -z "$UPLOADTARGET" ] ; then
			echo "$(date +%s);${UPLOADURL}${UPLOADTO}s25rttr_$VERSION-${REVISION}_$ARCH$FORMAT" >> ${UPLOADFILE}rapidshare.txt
		fi
	fi
	
	echo "creating new updater tree"

	# fastcopy files (only dirs and files, no symlinks
	(cd $ARCHNEWDIR/unpacked/s25rttr_$VERSION && find -type d -a ! -path */dbg* -exec mkdir -vp $ARCHNEWDIR/updater/{} \;)
	(cd $ARCHNEWDIR/unpacked/s25rttr_$VERSION && find -type f -a ! -name *.dbg -exec cp {} $ARCHNEWDIR/updater/{} \;)
	
	# note symlinks
	L=/tmp/links.$$
	echo -n > $L
	echo "reading links"
	(cd $ARCHNEWDIR/unpacked/s25rttr_$VERSION && find -type l -exec bash -c 'echo "{} $(readlink {})"' \;) | tee $L
	
	# savegame version
	UPDATERPATH=$(dirname $(find $ARCHNEWDIR/updater -name "s25update*" | head -n 1))
	S=/tmp/savegameversion.$$
	echo "reading savegame version"
	echo $SAVEGAMEVERSION > $S	
	cp -v $S $UPDATERPATH/savegameversion || exit 1	
	
	# note hashes
	F=/tmp/files.$$
	echo "reading files"
	(cd $ARCHNEWDIR/updater && md5deep -r -l .) | tee $F

	# bzip files
	find $ARCHNEWDIR/updater -type f -exec bzip2 -v {} \;
	
	# move file lists
	mv -v $L $ARCHNEWDIR/updater/links || exit 1
	mv -v $F $ARCHNEWDIR/updater/files || exit 1
	mv -v $S $ARCHNEWDIR/updater/savegameversion || exit 1	

	# create human version notifier
	echo "$REVISION" > $ARCHNEWDIR/revision-${REVISION} || exit 1
	echo "$VERSION"  > $ARCHNEWDIR/version-$VERSION || exit 1
	echo "$REVISION" > $ARCHNEWDIR/revision || exit 1
	echo "$VERSION"  > $ARCHNEWDIR/version || exit 1
	echo "${VERSION}-${REVISION}" > $ARCHNEWDIR/full-version || exit 1

	
	# rotate trees
	rm -rf $ARCHDIR.5
	mv $ARCHDIR.4 $ARCHDIR.5
	mv $ARCHDIR.3 $ARCHDIR.4
	mv $ARCHDIR.2 $ARCHDIR.3
	mv $ARCHDIR.1 $ARCHDIR.2
	mv $ARCHDIR $ARCHDIR.1
	mv $ARCHNEWDIR $ARCHDIR || exit 1

	echo "done"
else
	echo "nothing changed - no update necessary"
fi

cleanup
exit 0
