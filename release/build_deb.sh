#!/bin/bash

cd $(dirname $0)/../

source release/repository.def || error

if [ ! -d "$REPOSITORY" ] ; then
	echo "ERROR: repository.def does not contain REPOSITORY"
	exit 1
fi

if [ ! -z "$DISTRIBUTION" ] ; then
	echo "ERROR: repository.def does not contain DISTRIBUTION"
	exit 1
fi

debian/update-changelog.sh || exit 1

VERSION=$(head debian/changelog -n 1 | cut -d '-' -f 1 | cut -d '(' -f 2)
REVISION=$(head debian/changelog -n 1 | cut -d '-' -f 2 | cut -d ')' -f 1)

PARAMS="--svn-ignore-new -k6D09334C"

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
PARAMS="-b $DISTRIBUTION $*"

# include files
reprepro $PARAMS -S games -P optional includedsc $DISTRIBUTION release/deb/s25rttr_${VERSION}-${REVISION}.dsc || exit 1
reprepro $PARAMS include $DISTRIBUTION release/deb/s25rttr_${VERSION}-${REVISION}_i386.changes || exit 1
reprepro $PARAMS include $DISTRIBUTION release/deb/s25rttr_${VERSION}-${REVISION}_amd64.changes || exit 1

#svn ci debian/changelog -m "Automatic debian/changelog update"

exit 0
