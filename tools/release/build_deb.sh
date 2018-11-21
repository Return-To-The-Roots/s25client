#!/bin/bash

cd $(dirname $0)/../

source release/repository.def || error

# pass arguments (build number) to update-changelog.sh
debian/update-changelog.sh $* || exit 1

VERSION=$(head debian/changelog -n 1 | cut -d '-' -f 1 | cut -d '(' -f 2)
REVISION=$(head debian/changelog -n 1 | cut -d '-' -f 2 | cut -d ')' -f 1)

PARAMS="--svn-ignore-new -k6D09334C"

mkdir -p ../tarballs
 
if [ ! -f "../tarballs/s25rttr_${VERSION}.orig.tar.gz" ] ; then
	rm -rf ../tarballs/s25rttr_${VERSION}
	svn export . ../tarballs/s25rttr_${VERSION}
	rm -r ../tarballs/s25rttr_${VERSION}/debian
	rm -r ../tarballs/s25rttr_${VERSION}/contrib
	(cd ../tarballs && tar cvzf s25rttr_${VERSION}.orig.tar.gz s25rttr_${VERSION})
	rm -r ../tarballs/s25rttr_${VERSION}
	PARAMS="-sa $PARAMS"

	find ../tarballs -mtime +7 -exec rm {} \;
	rm -rf ../build-area/*
fi

if [ ! -z "$UPLOAD" ] ; then
	echo "Building Source only for upload to $UPLOAD"
	svn rm --force contrib
	svn-buildpackage $PARAMS -S || exit 1

	dput $UPLOAD ../build-area/s25rttr*${VERSION}-${REVISION}*.changes || exit 1
else
	if [ ! -d "$REPOSITORY" ] ; then
		echo "ERROR: repository.def does not contain REPOSITORY"
		exit 1
	fi

	if [ -z "$DISTRIBUTION" ] ; then
		echo "ERROR: repository.def does not contain DISTRIBUTION"
		exit 1
	fi

	echo "Building Binary packages"
	mkdir -p release/deb

	# build source, i386 and all
	svn-buildpackage $PARAMS -ai386 || exit 1
	mv -v ../build-area/s25rttr_${VERSION}-${REVISION}.dsc release/deb || exit 1
	mv -v ../build-area/s25rttr_${VERSION}-${REVISION}.tar.gz release/deb || exit 1
	mv -v ../build-area/s25rttr_${VERSION}-${REVISION}_i386.changes release/deb || exit 1
	mv -v ../build-area/s25rttr*_${VERSION}-${REVISION}_i386.deb release/deb || exit 1
	mv -v ../build-area/s25rttr*_${VERSION}-${REVISION}_all.deb release/deb || exit 1

	# build amd64
	svn-buildpackage $PARAMS -aamd64 -B || exit 1
	mv -v ../build-area/s25rttr_${VERSION}-${REVISION}_amd64.changes release/deb || exit 1
	mv -v ../build-area/s25rttr*_${VERSION}-${REVISION}_amd64.deb release/deb || exit 1

	# add repository to params
	PARAMS="-b $REPOSITORY"

	#echo "reprepro $PARAMS list hardy s25rttr | grep -q ${VERSION}-${REVISION}"
	reprepro $PARAMS list $DISTRIBUTION s25rttr | grep -q ${VERSION}-${REVISION}
	EXIT=$?
	#echo $EXIT
	if [ "$EXIT" = "0" ] ; then
		echo "removing s25rttr from repository to avoid conflicts"
		reprepro $PARAMS remove $DISTRIBUTION s25rttr s25rttr-music s25rttr-maps s25rttr-common || exit 1
	fi

	# include files
	reprepro $PARAMS -S games -P optional includedsc $DISTRIBUTION release/deb/s25rttr_${VERSION}-${REVISION}.dsc || exit 1
	reprepro $PARAMS include $DISTRIBUTION release/deb/s25rttr_${VERSION}-${REVISION}_i386.changes || exit 1
	reprepro $PARAMS include $DISTRIBUTION release/deb/s25rttr_${VERSION}-${REVISION}_amd64.changes || exit 1
fi

#svn ci debian/changelog -m "Automatic debian/changelog update"

exit 0

