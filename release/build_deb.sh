#!/bin/bash

cd $(dirname $0)/../

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

# add to repository
PARAMS="-b release/deb/repository $*"
DISTRI=hardy

reprepro $PARAMS -S games -P optional includedsc $DISTRI release/deb/s25rttr_${VERSION}-${REVISION}.dsc || exit 1
reprepro $PARAMS include $DISTRI release/deb/s25rttr_${VERSION}-${REVISION}_i386.changes || exit 1
reprepro $PARAMS include $DISTRI release/deb/s25rttr_${VERSION}-${REVISION}_amd64.changes || exit 1

svn ci debian/changelog -m "Automatic debian/changelog update"

exit 0
