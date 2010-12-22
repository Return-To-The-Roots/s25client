#!/bin/bash

if [ -z "$BUILDDIR" ] ; then
	export BUILDDIR=../build
fi

if [ -z "$VERSION" ] ; then
    export VERSION=$(grep WINDOW_VERSION $BUILDDIR/build_version.h | cut -d ' ' -f 3 | cut -d \" -f 2)
fi

if [ -z "$TARGET" ] ; then
	export TARGET=""
fi

if [ -z "$ARCH" ] ; then
	export ARCH=unknown
fi

echo "Building in $BUILDDIR for $ARCH"

echo "Executing ./cmake.sh --prefix=. --arch=$CROSS$ARCH"

mkdir -vp $BUILDDIR
OLDDIR=$PWD
cd $BUILDDIR
../build/cmake.sh --prefix=. --arch=$CROSS$ARCH || exit 1
cd $OLDDIR
unset OLDDIR

# make
mkdir -vp $BUILDDIR/RTTR
make -j 2 -C $BUILDDIR || exit 1

# get arch
ARCH="$(grep COMPILEFOR $BUILDDIR/CMakeCache.txt | cut -d '=' -f 2 | head -n 1).$(grep COMPILEARCH $BUILDDIR/CMakeCache.txt | cut -d '=' -f 2 | head -n 1)"

# install
mkdir -vp $ARCH
sudo rm -r $ARCH/s25rttr_*
make -C $BUILDDIR install DESTDIR="$PWD/$ARCH/s25rttr_$VERSION" || exit 1

# remove/pack music ...
if   [ -d $ARCH/s25rttr_$VERSION/share/s25rttr/RTTR/MUSIC/SNG ] ; then
	tar -C $ARCH --exclude=.svn -cvjf $ARCH/s25rttr-music_$VERSION.tar.bz2 s25rttr_$VERSION/share/s25rttr/RTTR/MUSIC || exit 1
	rm -f $ARCH/s25rttr_$VERSION/share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG
elif [ -d $ARCH/s25rttr_$VERSION/RTTR/MUSIC/SNG ] ; then
	tar -C $ARCH --exclude=.svn -cvjf $ARCH/s25rttr-music_$VERSION.tar.bz2 s25rttr_$VERSION/RTTR/MUSIC || exit 1
	rm -f $ARCH/s25rttr_$VERSION/RTTR/MUSIC/SNG/SNG_*.OGG
elif [ -d $ARCH/s25rttr_$VERSION/s25client.app/Contents/MacOS/share/s25rttr/RTTR/MUSIC/SNG ] ; then
	tar -C $ARCH --exclude=.svn -cvjf $ARCH/s25rttr-music_$VERSION.tar.bz2 s25rttr_$VERSION/s25client.app/Contents/MacOS/share/s25rttr/RTTR/MUSIC || exit 1
	rm -f $ARCH/s25rttr_$VERSION/s25client.app/Contents/MacOS/share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG
fi

# do they differ?
NEW=1
if [ -d $ARCH/old.s25rttr ] ; then
	diff -qrN $ARCH/old.s25rttr $ARCH/s25rttr_$VERSION
	NEW=$?
fi

# create archive
if [ $NEW -eq 1 ] || [ ! -f $ARCH/old.s25rttr.tar.bz2 ] ; then
	echo "creating new archive"
	rm -f $ARCH/s25rttr_$VERSION.tar.bz2
	tar  -C $ARCH --exclude=.svn -cvjf $ARCH/s25rttr_$VERSION.tar.bz2 s25rttr_$VERSION || exit 1

	rm -f $ARCH/old.s25rttr.tar.bz2
	cp $ARCH/s25rttr_$VERSION.tar.bz2 $ARCH/old.s25rttr.tar.bz2
else
	echo "using old archive"
	cp $ARCH/old.s25rttr.tar.bz2 $ARCH/s25rttr_$VERSION.tar.bz2
fi

sudo rm -r $ARCH/old.s25rttr
mv $ARCH/s25rttr_$VERSION $ARCH/old.s25rttr

echo "Release Version $VERSION erstellt"

####
#
#
#
#
#sudo rm -v -r $ARCH
#mkdir -vp $ARCH
#if ! cd $ARCH ; then
#	exit 1
#fi
#
#mkdir -v -p s25rttr_$VERSION
#if ! cd s25rttr_$VERSION ; then
#	exit 1
#fi
#
#echo $PWD
#
#mkdir -vp bin
#mkdir -vp share/s25rttr
#mkdir -vp share/s25rttr/S2
#mkdir -vp share/s25rttr/driver/video
#mkdir -vp share/s25rttr/driver/audio
#mkdir -vp lib
#
#cp -v ../../../RTTR/texte/readme.txt .
#cp -v ../../../RTTR/texte/keyboardlayout.txt .
#
#rm -rf share/s25rttr/RTTR
#svn --force export ../../../RTTR share/s25rttr/RTTR
#
#cd  share/s25rttr
#
#rm -vf ../../../../s25rttr_music.tar.bz2
#tar --exclude=.svn -cvjf ../../../../s25rttr_music.tar.bz2 RTTR/MUSIC
#
#cd ../..
#
#cp -rv ../../../RTTR/languages/*.mo share/s25rttr/RTTR/languages/
#
#rm -vf share/s25rttr/RTTR/languages/*.po
#rm -vf share/s25rttr/RTTR/REPLAYS/*.rpl
#rm -vf share/s25rttr/RTTR/sound.lst
#rm -vf share/s25rttr/RTTR/settings.bin
#rm -vf share/s25rttr/RTTR/MUSIC/SNG/SNG_*.OGG
#rm -vf *~ *.bak
#
#rm -vrf share/s25rttr/S2
#mkdir -vp share/s25rttr/S2
#touch share/s25rttr/S2/put\ your\ S2-Installation\ in\ here
#
#if [ -f ../../files.$ARCH.sh ] ; then
#	../../files.$ARCH.sh
#fi
#
#cd ..
#
#rm -vf ../s25rttr_$VERSION.tar.bz2
#tar --exclude=.svn -cvjf ../s25rttr_$VERSION.tar.bz2 s25rttr_$VERSION
#
#echo "Release Version $VERSION erstellt"
#
#exit 0
