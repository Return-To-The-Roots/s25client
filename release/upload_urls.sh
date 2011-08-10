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
	echo "WARN: $RELEASEDEF does not contain TARGET, using $(pwd)"
	TARGET=$(pwd)
fi

# create changelog
svn log $(dirname $(svn info | grep "URL" | cut -d ' ' -f 2)) > ${UPLOADFILE}changelog.txt || exit 2

# upload data
scp -4 ${UPLOADFILE}rapidshare.txt $UPLOADTARGET || exit 3
scp -4 ${UPLOADFILE}changelog.txt $UPLOADTARGET || exit 4

if [ ! -z "$UPLOADSCRIPT" ] ; then
	echo "running post upload script"
	bash -c "$UPLOADSCRIPT"
fi

exit 0
