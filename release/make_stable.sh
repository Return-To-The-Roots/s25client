#!/bin/sh

cd $(dirname $0)

if [ -z "$ARCH" ] ; then
    export ARCH="local"
fi

export TARGET=/var/www/ra-doersch.de/nightly/s25client/stable
export VERSION=0.7

#sed "s/WINDOW_VERSION \".*\"/WINDOW_VERSION \"$(date +%Y%m%d)\"/" ../version.h > /tmp/version.$$
#if ! diff -s $BUILDDIR/build_version.h /tmp/version.$$ >/dev/null 2>&1 ; then
#	cp -v /tmp/version.$$ $BUILDDIR/version.h
#else
#	rm -f /tmp/version.$$
#fi

rm -fr $TARGET/$ARCH.new
mkdir -p $TARGET
mkdir -p $TARGET/$ARCH
mkdir -p $TARGET/$ARCH.new

mv $TARGET/build_${ARCH}.log $TARGET/build_${ARCH}_old.log

echo "Build started: $(date)" >$TARGET/build_${ARCH}.log
./make_release.sh >>$TARGET/build_${ARCH}.log 2>&1
EXIT=$?
echo "Build completed: $(date)" >>$TARGET/build_${ARCH}.log

if [ $EXIT != 0 ] ; then
	cat $TARGET/build_${ARCH}.log | /usr/bin/mail -s "Stable Build $VERSION Failed" -c bugs@siedler25.org sf-team@siedler25.org

	EXIT=1
else
	REVISION=$(grep WINDOW_REVISION $BUILDDIR/build_version.h | cut -d ' ' -f 3 | cut -d \" -f 2)
	rm -f $TARGET/s25rttr_${VERSION}-*_${ARCH}.tar.bz2
	rm -f $TARGET/s25rttr_*-${REVISION}_${ARCH}.tar.bz2
	cp -v ${ARCH}/s25rttr_$VERSION.tar.bz2 /srv/buildfarm/uploads/s25rttr_${VERSION}-${REVISION}_${ARCH}.tar.bz2 >> $TARGET/build_${ARCH}.log
	
	tar -C $TARGET/$ARCH.new --strip 1 -xf ${ARCH}/s25rttr-music_$VERSION.tar.bz2 >> $TARGET/build_${ARCH}.log
	tar -C $TARGET/$ARCH.new --strip 1 -xf ${ARCH}/s25rttr_$VERSION.tar.bz2 >> $TARGET/build_${ARCH}.log
	
	rm -f ${ARCH}/s25rttr_$VERSION.tar.bz2

	# for mailer-script
	echo "${REVISION}" > $TARGET/.revision
	echo "${VERSION}" > $TARGET/.version
	echo "s25rttr_${VERSION}-${REVISION}_${ARCH}.tar.bz2" >> $TARGET/.files
	
	OPWD=$PWD
	cd $TARGET/$ARCH.new
	echo -n > /tmp/links.$$
	find -type l -exec bash -c 'echo "{} $(readlink {})" >> /tmp/links.$$ ; rm {}' \;
	md5deep -r -l . > /tmp/files.$$
	cd $OPWD

	find $TARGET/$ARCH.new -type f -exec bzip2 -v {} >> $TARGET/build_${ARCH}.log 2>&1 \;

	mv /tmp/files.$$ $TARGET/$ARCH.new/files
	mv /tmp/links.$$ $TARGET/$ARCH.new/links
	
	touch $TARGET/$ARCH.new/revision-$REVISION
	touch $TARGET/$ARCH.new/version-$VERSION

	# swap old & new ARCH-Tree
	rm -fr $TARGET/$ARCH.old
	mv $TARGET/$ARCH $TARGET/$ARCH.old
	mv $TARGET/$ARCH.new $TARGET/$ARCH
	
	echo "New updater-tree created for $VERSION-$REVISION" >> $TARGET/build_${ARCH}.log
	
	EXIT=0
fi

unset VERSION

exit $EXIT
