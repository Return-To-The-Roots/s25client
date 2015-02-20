#!/bin/bash

TYPE=$1
SRCDIR=$2

usage()
{
	echo "$0 [nightly|stable] srcdir"
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
	cleanup
	exit 1
}

if [ -z "$TYPE" ] ; then
	usage
fi

if [ ! -d "$SRCDIR" ] ; then
	usage
fi

RELEASEDEF=$SRCDIR/release/release.$TYPE.def
source $RELEASEDEF || error

if [ ! -d "$TARGET" ] ; then
	echo "WARN: $RELEASEDEF does not contain TARGET, using $(pwd)"
	TARGET=$(pwd)
fi

# get arch
ARCH="$(grep COMPILEFOR CMakeCache.txt | cut -d '=' -f 2 | head -n 1).$(grep COMPILEARCH CMakeCache.txt | cut -d '=' -f 2 | head -n 1)"

# current and new package directory
ARCHDIR=$TARGET/$ARCH
ARCHNEWDIR=$TARGET/$ARCH.new

rm -rf $ARCHNEWDIR
mkdir -p $ARCHNEWDIR
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

make || error

# get version
VERSION=$(grep WINDOW_VERSION build_version.h | cut -d ' ' -f 3 | cut -d \" -f 2)

# get revision
REVISION=$(grep WINDOW_REVISION build_version.h | cut -d ' ' -f 3 | cut -d \" -f 2)

if [ $REVISION -eq 0 ] ; then
	echo "error: revision is null"
	error
fi

echo "current version is: $VERSION-$REVISION"

DESTDIR=$ARCHNEWDIR/unpacked/s25rttr_$VERSION
make install DESTDIR=$DESTDIR || error

# do they differ?
CHANGED=1
if [ ! "$FORCE" = "1" ] && [ -d $ARCHDIR/unpacked/s25rttr_$VERSION ] ; then
	diff -qrN $ARCHDIR/unpacked/s25rttr_$VERSION $DESTDIR
	CHANGED=$?
fi

if [ "$FORCE" = "1" ] ; then
	echo "FORCE is set - forcing update"
fi

# create packed data and updater
if [ $CHANGED -eq 1 ] || [ ! -f $ARCHDIR/packed/s25rttr.tar.bz2 ] ; then
	echo "creating new archive"

	# remove old build artefacts
	rm -f ../s25rttr*.tar.bz2
	
	# pack
	tar -C $ARCHNEWDIR/unpacked \
		--exclude=.svn \
		--exclude=dbg \
		--exclude s25rttr_$VERSION/share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG \
		--exclude s25rttr_$VERSION/RTTR/MUSIC/SNG/SNG_*.OGG \
		--exclude s25rttr_$VERSION/s25client.app/Contents/MacOS/share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG \
		-cvjf $ARCHNEWDIR/packed/s25rttr.tar.bz2 s25rttr_$VERSION || error
	
	cp -v $ARCHNEWDIR/packed/s25rttr.tar.bz2 ../s25rttr_$VERSION-${REVISION}_$ARCH.tar.bz2 || exit

	if [ -d $ARCHNEWDIR/unpacked/s25rttr_$VERSION/dbg ] ; then
		tar -C $ARCHNEWDIR/unpacked \
			-cvjf $ARCHNEWDIR/packed/s25rttr_dbg.tar.bz2 s25rttr_$VERSION/dbg || error
		cp -v $ARCHNEWDIR/packed/s25rttr_dbg.tar.bz2 ../s25rttr-dbg_$VERSION-${REVISION}_$ARCH.tar.bz2 || exit 1 
	else
		touch ../s25rttr-dbg_$VERSION-${REVISION}_$ARCH.tar.bz2
	fi
	
	# link to archive
	mkdir -p $ARCHIVE
	ln -v $ARCHNEWDIR/packed/s25rttr.tar.bz2 $ARCHIVE/s25rttr_$VERSION-${REVISION}_$ARCH.tar.bz2 || \
		cp -v $ARCHNEWDIR/packed/s25rttr.tar.bz2 $ARCHIVE/s25rttr_$VERSION-${REVISION}_$ARCH.tar.bz2 || exit 1

	if [ -d $ARCHNEWDIR/unpacked/s25rttr_$VERSION/dbg ] ; then
		ln -v $ARCHNEWDIR/packed/s25rttr_dbg.tar.bz2 $ARCHIVE/s25rttr-dbg_$VERSION-${REVISION}_$ARCH.tar.bz2 || \
			cp -v $ARCHNEWDIR/packed/s25rttr_dbg.tar.bz2 $ARCHIVE/s25rttr-dbg_$VERSION-${REVISION}_$ARCH.tar.bz2 || exit 1
	fi

	# do upload
	if [ ! "$NOUPLOAD" = "1" ] && [ ! -z "$UPLOADTARGET" ] ; then
		if [ -z "$UPLOADTO" ] ; then
			UPLOADTO="$VERSION/"
		fi
		
		echo "uploading file to $UPLOADTARGET$UPLOADTO"
		ssh $UPLOADHOST "mkdir -vp $UPLOADTO" || echo "mkdir $UPLOADTO failed"
		rsync -avz --progress $ARCHIVE/s25rttr_$VERSION-${REVISION}_$ARCH.tar.bz2 $UPLOADTARGET$UPLOADTO || echo "scp failed"
		if [ ! -z "$UPLOADTARGET" ] ; then
			echo "${UPLOADURL}${UPLOADTO}s25rttr_$VERSION-${REVISION}_$ARCH.tar.bz2" >> ${UPLOADFILE}rapidshare.txt
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
	
	# note hashes
	F=/tmp/files.$$
	echo "reading files"
	(cd $ARCHNEWDIR/updater && md5deep -r -l .) | tee $F

	# bzip files
	find $ARCHNEWDIR/updater -type f -exec bzip2 -v {} \;
	
	# move file lists
	mv -v $L $ARCHNEWDIR/updater/links || exit 1
	mv -v $F $ARCHNEWDIR/updater/files || exit 1

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
