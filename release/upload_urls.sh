#!/bin/bash

TYPE=$1

usage()
{
	echo "$0 [nightly|stable]"
	exit 1
}

if [ -z "$TYPE" ] ; then
	usage
fi

cd $(dirname $0)

RELEASEDEF=release.$TYPE.def
source $RELEASEDEF || exit 1

if [ ! -d "$TARGET" ] ; then
	echo "WARN: $RELEASEDEF does not contain $TARGET, using $(pwd)"
	TARGET=$(pwd)
fi

# create changelog
svn log . > ${UPLOADURL}changelog.txt

# upload data
scp ${UPLOADURL}rapidshare.txt $UPLOADTARGET
scp ${UPLOADURL}changelog.txt $UPLOADTARGET

exit 0
