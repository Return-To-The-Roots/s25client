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
echo "Creating Changelog ..."
if [ -e .svn ] ; then
	svn log $(dirname $(svn info | grep "URL" | cut -d ' ' -f 2)) > ${UPLOADFILE}changelog.txt || exit 2
elif [ -e .git ] ; then
	git log > ${UPLOADFILE}changelog.txt || exit 2
else
	echo "No changelog" > ${UPLOADFILE}changelog.txt
fi

cp ${UPLOADFILE}changelog.txt changelog-$TYPE.txt
cp ${UPLOADFILE}rapidshare.txt rapidshare-$TYPE.txt

# upload data
echo "Uploading Data ..."
scp -4 ${UPLOADFILE}rapidshare.txt $UPLOADTARGET$UPLOADTO || exit 3
scp -4 ${UPLOADFILE}changelog.txt $UPLOADTARGET$UPLOADTO || exit 4

if [ ! -z "$UPLOADSCRIPT" ] ; then
	echo "Running post upload script ..."
	bash -c "$UPLOADSCRIPT"
fi

exit 0
